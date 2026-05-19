from __future__ import annotations

import asyncio
import statistics
import time
import uuid
from typing import Any

import psutil

from config import get_settings
from core.audit_log import AuditLog
from core.database import DatabaseService
from core.models import BenchmarkCapture, BenchmarkResult
from core.optimizer import SystemOptimizer
from core.performance_diagnostics import FrameTimeCapture
from core.tools.collector import MetricCollector


class BenchmarkEngine:
    """Capture measurable before/after evidence for safe tweak sets."""

    MIN_DURATION_SECONDS = 2

    def __init__(
        self,
        *,
        database: DatabaseService,
        audit_log: AuditLog,
        optimizer: SystemOptimizer,
        collector: MetricCollector,
        network_optimizer: Any | None = None,
        gpu_controller: Any | None = None,
        frame_time_capture: FrameTimeCapture | None = None,
    ) -> None:
        self.database = database
        self.audit_log = audit_log
        self.optimizer = optimizer
        self.collector = collector
        self.network_optimizer = network_optimizer
        self.gpu_controller = gpu_controller
        self.frame_time_capture = frame_time_capture or FrameTimeCapture(get_settings().presentmon_csv_path)
        self._running = False

    @property
    def running(self) -> bool:
        return self._running

    async def run(
        self,
        orchestrator,
        *,
        workload_name: str,
        tweak_set: list[str] | None = None,
        duration_seconds: int = 6,
        notes: str = "",
        revert_after: bool = True,
    ) -> dict[str, Any]:
        if self._running:
            return {
                "success": False,
                "error": "A benchmark run is already in progress.",
                "unsupported": False,
            }

        self._running = True
        tweak_set = list(dict.fromkeys(tweak_set or []))
        duration_seconds = max(self.MIN_DURATION_SECONDS, int(duration_seconds))
        apply_results: list[dict[str, Any]] = []
        revert_results: list[dict[str, Any]] = []
        applied_snapshot_ids: list[tuple[str, str]] = []

        try:
            baseline = await self._capture_window(duration_seconds, orchestrator=orchestrator)
            for tweak_id in tweak_set:
                result = await self.optimizer.apply_tweak(
                    tweak_id,
                    snapshot=orchestrator.last_snapshot,
                    triggered_by="benchmark",
                )
                apply_entry = {"tweak_id": tweak_id, **result}
                apply_results.append(apply_entry)
                if result.get("success") and result.get("revert_snapshot_id"):
                    applied_snapshot_ids.append((tweak_id, result["revert_snapshot_id"]))

            optimized = await self._capture_window(duration_seconds, orchestrator=orchestrator)

            if revert_after:
                for tweak_id, snapshot_id in reversed(applied_snapshot_ids):
                    revert_result = await self.optimizer.revert_tweak(
                        tweak_id=tweak_id,
                        snapshot_id=snapshot_id,
                        triggered_by="benchmark",
                    )
                    revert_results.append({"tweak_id": tweak_id, **revert_result})

            benchmark = self._build_result(
                workload_name=workload_name,
                duration_seconds=duration_seconds,
                tweak_set=tweak_set,
                session_id=(orchestrator.current_state.get("active_session") or {}).get("id"),
                baseline=baseline,
                optimized=optimized,
                notes=notes,
                apply_results=apply_results,
                revert_results=revert_results,
            )
            await self.database.insert_benchmark_result(benchmark)
            await self.audit_log.record_event(
                module="benchmark",
                action="run",
                target=workload_name,
                before_value={
                    "tweak_set": tweak_set,
                    "duration_seconds": duration_seconds,
                },
                after_value={
                    "benchmark_id": benchmark.benchmark_id,
                    "verdict": benchmark.verdict,
                    "unsupported_reasons": benchmark.unsupported_reasons,
                },
                rationale="Captured a before/after benchmark window to measure the effect of safe PulseBoost tweaks.",
                validity_tag="VALIDATED",
                triggered_by="user",
                status="success",
            )
            return {"success": True, "result": benchmark.to_dict()}
        finally:
            self._running = False

    async def results(self, limit: int = 25) -> list[dict[str, Any]]:
        return await self.database.list_benchmark_results(limit=limit)

    async def result(self, benchmark_id: str) -> dict[str, Any] | None:
        return await self.database.get_benchmark_result(benchmark_id)

    async def _capture_window(self, duration_seconds: int, *, orchestrator: Any | None = None) -> BenchmarkCapture:
        sample_count = max(2, int(duration_seconds))
        cpu_samples: list[float] = []
        latest_snapshot: Any | None = None
        unsupported_reasons = [
            "Average FPS capture is unavailable because no trusted frame-time capture source is configured for Benchmark Mode.",
            "1% low FPS capture is unavailable because no trusted frame-time capture source is configured for Benchmark Mode.",
            "Frame-time evidence is unavailable because no trusted frame-time capture source is configured for Benchmark Mode.",
            "GPU utilization capture is unavailable until a supported GPU telemetry runtime is active for this machine.",
            "Ping/jitter capture is unavailable until network diagnostics are enabled.",
        ]
        frame_marker = self.frame_time_capture.snapshot_marker()

        # Prime psutil's non-blocking sampler once so short benchmark windows remain responsive.
        psutil.cpu_percent(interval=None)
        for index in range(sample_count):
            latest_snapshot = self._sample_snapshot()
            cpu_samples.append(self._sample_cpu_percent(latest_snapshot))
            if index < sample_count - 1:
                await asyncio.sleep(1.0)

        unstable = False
        if len(cpu_samples) >= 2:
            cpu_stdev = statistics.pstdev(cpu_samples)
            unstable = cpu_stdev >= 18.0

        frame_time = self.frame_time_capture.capture(
            foreground_app=self._resolve_foreground_app(latest_snapshot, orchestrator),
            session_mode="benchmark",
            since_marker=frame_marker,
        )
        if frame_time.get("supported"):
            unsupported_reasons = [
                reason for reason in unsupported_reasons if "FPS capture" not in reason and "Frame-time evidence" not in reason
            ]
        elif frame_time.get("reason"):
            unsupported_reasons = [
                reason for reason in unsupported_reasons if "FPS capture" not in reason and "Frame-time evidence" not in reason
            ]
            unsupported_reasons.extend(
                [
                    f"Average FPS capture is unavailable: {frame_time['reason']}",
                    f"1% low FPS capture is unavailable: {frame_time['reason']}",
                    f"Frame-time evidence is unavailable: {frame_time['reason']}",
                ]
            )

        ping_ms = None
        jitter_ms = None
        if self.network_optimizer:
            diagnostics = await self.network_optimizer.diagnostics(
                session_mode="benchmark",
                active_session=None,
                probe_attempts=1,
                probe_timeout_seconds=0.5,
            )
            public_probe = diagnostics.get("targets", {}).get("public", {})
            if public_probe.get("supported"):
                ping_ms = public_probe.get("latency_ms")
                jitter_ms = public_probe.get("jitter_ms")
                unsupported_reasons = [
                    reason for reason in unsupported_reasons if "Ping/jitter" not in reason
                ]

        gpu_percent = None
        if self.gpu_controller:
            gpu_stats = await self.gpu_controller.stats()
            if gpu_stats.get("telemetry_supported") and gpu_stats.get("utilization_percent") is not None:
                gpu_percent = gpu_stats.get("utilization_percent")
                unsupported_reasons = [
                    reason for reason in unsupported_reasons if "GPU utilization" not in reason
                ]
            elif gpu_stats.get("reason"):
                unsupported_reasons = [
                    reason for reason in unsupported_reasons if "GPU utilization" not in reason
                ]
                unsupported_reasons.append(f"GPU utilization capture is unavailable: {gpu_stats['reason']}")

        return BenchmarkCapture(
            avg_fps=frame_time.get("fps_average"),
            one_percent_low_fps=frame_time.get("fps_1_low"),
            average_frame_time_ms=frame_time.get("average_frame_time_ms"),
            p95_frame_time_ms=frame_time.get("p95_frame_time_ms"),
            frame_time_variance_ms=frame_time.get("frame_time_variance_ms"),
            frametime_supported=bool(frame_time.get("supported")),
            frametime_source=frame_time.get("source"),
            frametime_reason=frame_time.get("reason"),
            cpu_percent=round(sum(cpu_samples) / len(cpu_samples), 2),
            gpu_percent=gpu_percent,
            ping_ms=ping_ms,
            jitter_ms=jitter_ms,
            sample_count=len(cpu_samples),
            unstable=unstable,
            unsupported_reasons=unsupported_reasons,
        )

    def _sample_snapshot(self) -> Any:
        if hasattr(self.collector, "get_snapshot"):
            return self.collector.get_snapshot()
        try:
            return self.collector.get_snapshot()
        except Exception:  # pragma: no cover - fallback path for unusual runtimes
            return None

    def _sample_cpu_percent(self, snapshot: Any | None) -> float:
        try:
            if snapshot is not None:
                return float(getattr(snapshot, "cpu_total"))
        except (TypeError, ValueError, AttributeError):
            pass
        try:
            return float(psutil.cpu_percent(interval=None))
        except Exception:  # pragma: no cover - fallback path for unusual runtimes
            fallback_snapshot = self.collector.get_snapshot()
            return float(getattr(fallback_snapshot, "cpu_total", 0.0))

    def _resolve_foreground_app(self, snapshot: Any | None, orchestrator: Any | None) -> dict[str, Any] | None:
        foreground = getattr(snapshot, "foreground_app", None)
        if foreground:
            return foreground
        last_snapshot = getattr(orchestrator, "last_snapshot", None)
        if last_snapshot is not None:
            return getattr(last_snapshot, "foreground_app", None)
        return None

    def _build_result(
        self,
        *,
        workload_name: str,
        duration_seconds: int,
        tweak_set: list[str],
        session_id: str | None,
        baseline: BenchmarkCapture,
        optimized: BenchmarkCapture,
        notes: str,
        apply_results: list[dict[str, Any]],
        revert_results: list[dict[str, Any]],
    ) -> BenchmarkResult:
        avg_fps_delta = self._delta(optimized.avg_fps, baseline.avg_fps)
        one_percent_low_delta = self._delta(optimized.one_percent_low_fps, baseline.one_percent_low_fps)
        frame_time_variance_delta = self._delta(optimized.frame_time_variance_ms, baseline.frame_time_variance_ms)
        fps_delta_percent = self._percent_delta(optimized.avg_fps, baseline.avg_fps)
        p95_frame_time_delta_percent = self._percent_delta(optimized.p95_frame_time_ms, baseline.p95_frame_time_ms)
        cpu_delta = self._delta(optimized.cpu_percent, baseline.cpu_percent)
        gpu_delta = self._delta(optimized.gpu_percent, baseline.gpu_percent)
        ping_delta = self._delta(optimized.ping_ms, baseline.ping_ms)
        jitter_delta = self._delta(optimized.jitter_ms, baseline.jitter_ms)
        frametime_supported = baseline.frametime_supported and optimized.frametime_supported
        frame_time_reason = baseline.frametime_reason or optimized.frametime_reason

        supported_metrics = {
            "avg_fps": avg_fps_delta is not None,
            "one_percent_low_fps": one_percent_low_delta is not None,
            "p95_frame_time_ms": baseline.p95_frame_time_ms is not None and optimized.p95_frame_time_ms is not None,
            "frame_time_variance_ms": frame_time_variance_delta is not None,
            "cpu_percent": cpu_delta is not None,
            "gpu_percent": gpu_delta is not None,
            "ping_ms": ping_delta is not None,
            "jitter_ms": jitter_delta is not None,
        }
        unsupported_reasons = list(dict.fromkeys([*baseline.unsupported_reasons, *optimized.unsupported_reasons]))
        verdict = self._verdict(
            baseline=baseline,
            optimized=optimized,
            avg_fps_delta=avg_fps_delta,
            one_percent_low_delta=one_percent_low_delta,
            frame_time_variance_delta=frame_time_variance_delta,
            fps_delta_percent=fps_delta_percent,
            p95_frame_time_delta_percent=p95_frame_time_delta_percent,
            cpu_delta=cpu_delta,
            ping_delta=ping_delta,
            jitter_delta=jitter_delta,
        )

        final_notes = notes.strip()
        if unsupported_reasons:
            detail = " ".join(unsupported_reasons)
            final_notes = f"{final_notes} {detail}".strip()

        return BenchmarkResult(
            benchmark_id=str(uuid.uuid4()),
            workload_name=workload_name,
            created_at=time.time(),
            duration_seconds=duration_seconds,
            tweak_set=tweak_set,
            session_id=session_id,
            baseline=baseline.to_dict(),
            optimized=optimized.to_dict(),
            avg_fps_delta=avg_fps_delta,
            one_percent_low_delta=one_percent_low_delta,
            frame_time_variance_delta=frame_time_variance_delta,
            fps_delta_percent=fps_delta_percent,
            p95_frame_time_delta_percent=p95_frame_time_delta_percent,
            cpu_delta=cpu_delta,
            gpu_delta=gpu_delta,
            ping_delta=ping_delta,
            jitter_delta=jitter_delta,
            frametime_supported=frametime_supported,
            frametime_source=baseline.frametime_source or optimized.frametime_source,
            baseline_fps_average=baseline.avg_fps,
            optimized_fps_average=optimized.avg_fps,
            baseline_fps_1_low=baseline.one_percent_low_fps,
            optimized_fps_1_low=optimized.one_percent_low_fps,
            baseline_average_frame_time_ms=baseline.average_frame_time_ms,
            optimized_average_frame_time_ms=optimized.average_frame_time_ms,
            baseline_p95_frame_time_ms=baseline.p95_frame_time_ms,
            optimized_p95_frame_time_ms=optimized.p95_frame_time_ms,
            baseline_frame_time_variance_ms=baseline.frame_time_variance_ms,
            optimized_frame_time_variance_ms=optimized.frame_time_variance_ms,
            frame_time_reason=frame_time_reason,
            notes=final_notes,
            verdict=verdict,
            supported_metrics=supported_metrics,
            unsupported_reasons=unsupported_reasons,
            apply_results=apply_results,
            revert_results=revert_results,
        )

    def _verdict(
        self,
        *,
        baseline: BenchmarkCapture,
        optimized: BenchmarkCapture,
        avg_fps_delta: float | None,
        one_percent_low_delta: float | None,
        frame_time_variance_delta: float | None,
        fps_delta_percent: float | None,
        p95_frame_time_delta_percent: float | None,
        cpu_delta: float | None,
        ping_delta: float | None,
        jitter_delta: float | None,
    ) -> str:
        if baseline.unstable or optimized.unstable:
            return "UNSTABLE"

        helped = 0
        regressed = 0

        if avg_fps_delta is not None:
            if avg_fps_delta >= 2.0:
                helped += 2
            elif avg_fps_delta <= -2.0:
                regressed += 2
        if fps_delta_percent is not None:
            if fps_delta_percent >= 2.0:
                helped += 1
            elif fps_delta_percent <= -2.0:
                regressed += 1
        if one_percent_low_delta is not None:
            if one_percent_low_delta >= 1.0:
                helped += 2
            elif one_percent_low_delta <= -1.0:
                regressed += 2
        if p95_frame_time_delta_percent is not None:
            if p95_frame_time_delta_percent <= -3.0:
                helped += 2
            elif p95_frame_time_delta_percent >= 3.0:
                regressed += 2
        if frame_time_variance_delta is not None:
            if frame_time_variance_delta <= -0.75:
                helped += 1
            elif frame_time_variance_delta >= 0.75:
                regressed += 1
        if cpu_delta is not None:
            if cpu_delta <= -5.0:
                helped += 1
            elif cpu_delta >= 5.0:
                regressed += 1
        if ping_delta is not None:
            if ping_delta <= -5.0:
                helped += 1
            elif ping_delta >= 5.0:
                regressed += 1
        if jitter_delta is not None:
            if jitter_delta <= -2.0:
                helped += 1
            elif jitter_delta >= 2.0:
                regressed += 1

        if regressed > helped:
            return "REGRESSION"
        if helped > regressed and helped > 0:
            return "HELPED"
        return "NO_MEASURABLE_IMPACT"

    def _delta(self, current: float | None, baseline: float | None) -> float | None:
        if current is None or baseline is None:
            return None
        return round(current - baseline, 2)

    def _percent_delta(self, current: float | None, baseline: float | None) -> float | None:
        if current is None or baseline in (None, 0):
            return None
        return round(((current - baseline) / baseline) * 100.0, 2)






