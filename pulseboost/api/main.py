"""PulseBoost FastAPI entrypoint."""
from __future__ import annotations

import asyncio
import logging
import os
import time
from contextlib import asynccontextmanager, suppress
from datetime import date
from functools import partial
from pathlib import Path

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware

from api.routes import router
from config import get_settings
from core.adaptive_engine import AdaptiveEngine
from core.agents.orchestrator import Orchestrator
from core.auth_service import AuthService
from core.audit_log import AuditLog
from core.benchmark_engine import BenchmarkEngine
from core.capabilities import CapabilityService
from core.compatibility import CompatibilityService
from core.cognition.plans import features_for_plan, normalize_plan
from core.database import DatabaseService
from core.event_bus import EventBus
from core.game_detection import GameDetector
from core.game_library import InstalledGameScanner
from core.game_profile_service import GameProfileService
from core.gpu_controller import GPUController
from core.metrics import MetricsService
from core.network_optimizer import NetworkOptimizer
from core.optimizer import SystemOptimizer
from core.platform_info import PlatformInfoService
from core.revert_manager import RevertManager
from core.safety_guard import SafetyGuard
from core.secure_storage import WindowsDPAPICipher
from core.session_manager import SessionManager
from core.session_recovery import SessionRecovery
from core.steam_library import SteamLibraryScanner
from core.trust_center import TrustCenterService

try:
    from anthropic import Anthropic
except ImportError:  # pragma: no cover
    Anthropic = None


logger = logging.getLogger(__name__)


class ConnectionManager:
    def __init__(self) -> None:
        self.connections: list[WebSocket] = []

    async def connect(self, websocket: WebSocket) -> None:
        await websocket.accept()
        self.connections.append(websocket)

    def disconnect(self, websocket: WebSocket) -> None:
        if websocket in self.connections:
            self.connections.remove(websocket)

    async def broadcast(self, payload: dict) -> None:
        disconnected: set[WebSocket] = set()
        for client_id, websocket in enumerate(self.connections):
            try:
                await websocket.send_json(payload)
            except Exception as e:
                logger.error(
                    f"WebSocket broadcast error | client={client_id} | "
                    f"type={type(e).__name__} | msg={e}"
                )
                disconnected.add(websocket)
        for websocket in disconnected:
            self.disconnect(websocket)


settings = get_settings()
manager = ConnectionManager()
_PROMPT_PATH = Path(__file__).resolve().parents[1] / "prompts" / "pulse_ai_system_prompt.txt"

PULSE_AI_SYSTEM_PROMPT = """
You are PulseAI, the intelligent assistant built into PulseBoost  a professional PC optimization application. You are knowledgeable, direct, and precise. You never hallucinate hardware specs or make up benchmark numbers.

You have real-time access to this user's system state (injected as context before each message):
- CPU: {cpu_name}, {cpu_cores} cores, current load {cpu_load}%
- RAM: {ram_total}GB total, {ram_used}GB used ({ram_percent}%)
- GPU: {gpu_name}, {gpu_temp}C, VRAM {gpu_vram_used}/{gpu_vram_total}GB
- Active game: {active_game} (profile: {game_profile})
- Current optimization profile: {opt_profile}
- Health score: {health_score}/100
- OS: Windows {windows_version}, build {build_number}
- Recent tweaks applied: {recent_tweaks}

You can help the user with:

1. PC OPTIMIZATION  Explain what tweaks do, why they help, when to apply them. Know Windows power plans, HPET, MSI mode, process priority, prefetch, superfetch, Game Mode, Hardware-Accelerated GPU Scheduling (HAGS), XMP/DOCP, pagefile settings, IRQ priority, network adapter settings (TCP buffer, Nagle algorithm, RSS), GPU driver settings (NVCP/AMD Software), and registry tweaks.

2. GAMING PERFORMANCE  FPS optimization for specific games, latency reduction, input lag, GPU/CPU bottleneck diagnosis, frame pacing, VSync vs G-Sync vs FreeSync, anti-cheat compatibility, overlay conflicts, shader pre-compilation.

3. HARDWARE KNOWLEDGE  CPU architectures (Intel 13th/14th/Arrow Lake, AMD Zen 4/5), GPU generations (RTX 40/50, RX 7000/9000), RAM timings and XMP profiles, NVMe vs SATA SSD performance, thermal paste and cooling, PSU headroom, PCIe bandwidth.

4. DIAGNOSTICS  Help interpret metrics shown in the app. If CPU load is consistently >85% at idle, suggest background process audit. If GPU temp >85C under load, suggest airflow or thermal paste. If RAM is >90% used, explain pagefile and potential upgrade path.

5. GENERAL TECH SUPPORT  Windows issues, driver problems, BSOD diagnosis (know common stop codes), disk health, network troubleshooting.

Rules:
- Always reference the user's actual hardware when relevant (use the injected context)
- If you don't know something specific, say so  never guess at benchmarks or compatibility
- Keep responses concise unless the user asks for detail
- If a fix exists within PulseBoost (a tweak, a profile, a setting), mention it by name
- Format: use short paragraphs. Use bullet points only for step-by-step instructions. No markdown headers in chat responses.
- Tone: like a smart friend who knows PCs deeply  not a corporate chatbot, not overly formal
""".strip()


def load_chat_system_prompt() -> str:
    try:
        text = _PROMPT_PATH.read_text(encoding="utf-8").strip()
        if text:
            return text
    except OSError:
        logger.warning("Could not read PulseAI system prompt from %s; using built-in fallback.", _PROMPT_PATH)
    return PULSE_AI_SYSTEM_PROMPT


class ChatTimeoutError(Exception):
    """Raised when upstream AI provider does not answer within deadline."""


async def call_claude(
    client: Anthropic,
    system_prompt: str,
    messages: list[dict[str, str]],
    model: str,
    max_tokens: int,
) -> object:
    loop = asyncio.get_running_loop()
    fn = partial(
        client.messages.create,
        model=model,
        max_tokens=max_tokens,
        system=system_prompt,
        messages=messages,
    )
    return await asyncio.wait_for(loop.run_in_executor(None, fn), timeout=30.0)


def resolved_desktop_runtime() -> str:
    runtime = os.getenv("DESKTOP_RUNTIME", settings.desktop_runtime).strip().lower()
    if runtime in {"electron", "desktop-electron"}:
        return "electron"
    if runtime in {"pywebview", "webview", "desktop-webview"}:
        return "pywebview"
    return runtime or "desktop-webview"


@asynccontextmanager
async def lifespan(app: FastAPI):
    database = DatabaseService(settings.resolved_db_path, use_pool=True)
    event_bus = EventBus()
    audit_log = AuditLog(database, event_bus)
    session_recovery = SessionRecovery(database, audit_log, event_bus)
    auth_service = AuthService(
        database=database,
        audit_log=audit_log,
        secret_cipher=WindowsDPAPICipher(),
        settings=settings,
    )
    desktop_runtime = resolved_desktop_runtime()
    platform_info = PlatformInfoService(settings.machine_id, settings.machine_name, desktop_runtime=desktop_runtime)
    capability_service = CapabilityService(database, platform_info)
    revert_manager = RevertManager(database, audit_log)
    compatibility = CompatibilityService()
    safety_guard = SafetyGuard()
    steam_scanner = SteamLibraryScanner()
    installed_game_scanner = InstalledGameScanner(steam_scanner=steam_scanner)
    game_detector = GameDetector(steam_scanner=steam_scanner, installed_game_scanner=installed_game_scanner)
    metrics_service = MetricsService()

    await database.initialize()
    await database.set_app_metadata("app_version", settings.app_version, value_type="string", source="runtime")
    await database.set_app_metadata("runtime", desktop_runtime, value_type="string", source="runtime")
    await auth_service.initialize()
    recovery = await session_recovery.begin_session(desktop_runtime)
    audit_log.set_session_id(recovery.session_id)
    revert_manager.set_session_id(recovery.session_id)
    capability_snapshot = await capability_service.refresh()
    await audit_log.record_event(
        module="capabilities",
        action="refresh",
        target=settings.machine_name,
        after_value=capability_snapshot.to_dict(),
        rationale="Captured startup capability snapshot for compatibility gating and trust reporting.",
        validity_tag="VALIDATED",
        triggered_by="startup",
        status="success",
    )

    optimizer = SystemOptimizer(
        database=database,
        audit_log=audit_log,
        revert_manager=revert_manager,
        safety_guard=safety_guard,
        capabilities=capability_snapshot,
    )
    await database.sync_tweak_catalog(optimizer.catalog(snapshot=None, session_mode="normal"))

    recovery_restore_results = []
    if recovery.recovery_required:
        recovery_restore_results = await optimizer.restore_temporary_tweaks(triggered_by="session_recovery")
        await audit_log.record_event(
            module="optimizer",
            action="restore_temporary_tweaks",
            target="session_recovery",
            before_value={"pending": len(recovery_restore_results)},
            after_value={"results": recovery_restore_results},
            rationale="Attempted cleanup of temporary tweaks after an unclean prior exit.",
            validity_tag="VALIDATED",
            triggered_by="startup",
            status="success",
        )

    session_manager = SessionManager(database, audit_log, game_detector)
    orchestrator = Orchestrator(manager.broadcast)
    await orchestrator.initialize()
    orchestrator.database = database
    orchestrator.account_service = auth_service
    orchestrator.executor.audit_log = audit_log
    orchestrator.executor.revert_manager = revert_manager
    orchestrator.executor.safety_guard = safety_guard
    orchestrator.executor.compatibility = compatibility
    orchestrator.executor.set_capabilities(capability_snapshot)
    orchestrator.session_manager = session_manager

    network_optimizer = NetworkOptimizer(database=database, audit_log=audit_log, capabilities=capability_snapshot)
    gpu_controller = GPUController(
        database=database,
        audit_log=audit_log,
        capabilities=capability_snapshot,
        hardware_profile=capability_service.last_profile.to_dict() if capability_service.last_profile else None,
    )
    benchmark_engine = BenchmarkEngine(
        database=database,
        audit_log=audit_log,
        optimizer=optimizer,
        collector=orchestrator.collector,
        network_optimizer=network_optimizer,
        gpu_controller=gpu_controller,
    )
    adaptive_engine = AdaptiveEngine(database=database, audit_log=audit_log, optimizer=optimizer)
    await adaptive_engine.initialize()
    orchestrator.adaptive_engine = adaptive_engine
    orchestrator.gpu_controller = gpu_controller
    game_profile_service = GameProfileService(
        database=database,
        optimizer=optimizer,
        steam_scanner=steam_scanner,
        installed_game_scanner=installed_game_scanner,
    )
    trust_center_service = TrustCenterService(
        database=database,
        capabilities=capability_snapshot,
        safety_guard=safety_guard,
        optimizer=optimizer,
    )

    async def broadcast_game_closed(payload: dict) -> None:
        await manager.broadcast(
            {
                "type": "game_closed",
                "timestamp": time.time(),
                **payload,
            }
        )

    game_close_watchdog_stop = asyncio.Event()

    async def game_close_watchdog() -> None:
        watched_process = ""
        watch_stop_event: asyncio.Event | None = None
        watch_task: asyncio.Task | None = None
        try:
            while not game_close_watchdog_stop.is_set():
                if watch_task and watch_task.done():
                    watch_task = None
                    watch_stop_event = None
                    watched_process = ""

                active_session = session_manager.current_session or {}
                process_hint = str(
                    active_session.get("executable_path")
                    or active_session.get("game_name")
                    or ""
                ).strip()
                process_name = os.path.basename(process_hint).strip().lower()

                if process_name and process_name != watched_process:
                    if watch_stop_event is not None:
                        watch_stop_event.set()
                    if watch_task is not None:
                        watch_task.cancel()
                        with suppress(asyncio.CancelledError):
                            await watch_task

                    watch_stop_event = asyncio.Event()
                    watched_process = process_name
                    watch_task = asyncio.create_task(
                        game_detector.monitor_game_close(
                            broadcast_game_closed,
                            active_process=process_name,
                            poll_interval_seconds=5.0,
                            stop_event=watch_stop_event,
                        )
                    )

                await asyncio.sleep(1.0)
        finally:
            if watch_stop_event is not None:
                watch_stop_event.set()
            if watch_task is not None:
                watch_task.cancel()
                with suppress(asyncio.CancelledError):
                    await watch_task

    app.state.orchestrator = orchestrator
    app.state.optimizer = optimizer
    app.state.metrics_service = metrics_service
    app.state.session_manager = session_manager
    app.state.auth_service = auth_service
    app.state.benchmark_engine = benchmark_engine
    app.state.adaptive_engine = adaptive_engine
    app.state.network_optimizer = network_optimizer
    app.state.gpu_controller = gpu_controller
    app.state.game_profile_service = game_profile_service
    app.state.trust_center_service = trust_center_service
    app.state.response_cache = {}
    app.state.foundation = {
        "database": database,
        "event_bus": event_bus,
        "audit_log": audit_log,
        "session_recovery": session_recovery,
        "capabilities": capability_service,
        "capability_snapshot": capability_snapshot.to_dict(),
        "hardware_profile": capability_service.last_profile.to_dict() if capability_service.last_profile else None,
        "recovery": recovery.to_dict(),
        "recovery_restore_results": recovery_restore_results,
        "runtime": desktop_runtime,
        "auth_service": auth_service,
        "migration_report": database.migration_report,
    }
    app.state.system_prompt = load_chat_system_prompt()

    task = asyncio.create_task(orchestrator.run())
    game_close_task = asyncio.create_task(game_close_watchdog())
    try:
        yield
    finally:
        game_close_watchdog_stop.set()
        await orchestrator.stop()
        shutdown_restore_results = await optimizer.restore_temporary_tweaks(triggered_by="shutdown_cleanup")
        if shutdown_restore_results:
            await audit_log.record_event(
                module="optimizer",
                action="restore_temporary_tweaks",
                target="shutdown_cleanup",
                before_value={"pending": len(shutdown_restore_results)},
                after_value={"results": shutdown_restore_results},
                rationale="Attempted cleanup of temporary tweaks during clean shutdown.",
                validity_tag="VALIDATED",
                triggered_by="shutdown",
                status="success",
            )
        task.cancel()
        game_close_task.cancel()
        with suppress(asyncio.CancelledError):
            await task
        with suppress(asyncio.CancelledError):
            await game_close_task
        for connection in list(manager.connections):
            with suppress(Exception):
                await connection.close()
            manager.disconnect(connection)
        await session_recovery.complete_session()
        await database.close()


origins = settings.cors_origin_list or ["http://localhost:5173", "http://localhost:8000"]

app = FastAPI(title="PulseBoost API", lifespan=lifespan)
app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,
    allow_methods=["*"],
    allow_headers=["*"],
)
app.include_router(router)


@app.get("/healthz")
async def healthz() -> dict:
    return {"ok": True}


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket) -> None:
    await manager.connect(websocket)
    orchestrator = websocket.app.state.orchestrator
    if orchestrator.current_state.get("metrics"):
        await websocket.send_json({"type": "full_state", **orchestrator.current_state})
    try:
        while True:
            payload = await websocket.receive_json()
            if payload.get("type") == "chat":
                await handle_chat(websocket, payload.get("message", ""), payload.get("history", []))
            elif payload.get("type") == "state_request":
                await websocket.send_json({"type": "full_state", **orchestrator.current_state})
            elif payload.get("type") == "optimization_decision":
                result = await orchestrator.decide_optimization(payload.get("optimization_id", ""), payload.get("decision", "dismiss"))
                await websocket.send_json({"type": "optimization_decision_result", "result": result})
    except WebSocketDisconnect:
        manager.disconnect(websocket)


async def handle_chat(websocket: WebSocket, message: str, history: list[dict]) -> None:
    orchestrator = websocket.app.state.orchestrator
    access_context = await websocket.app.state.auth_service.current_access_context()
    plan_name = normalize_plan(access_context["plan_tier"])
    features = features_for_plan(plan_name)
    today = date.today().isoformat()
    usage = await orchestrator.memory.increment_usage(f"chat.{plan_name}", today)
    if features.ai_chat_daily_limit is not None and usage > features.ai_chat_daily_limit:
        await stream_text(websocket, "Daily free chat limit reached. Upgrade the plan or reset the quota policy.")
        return

    state = orchestrator.current_state
    context_block = await build_chat_context_block(websocket, state)
    try:
        response = await generate_chat_response(
            message,
            history,
            state,
            chat_system_prompt=websocket.app.state.system_prompt,
            context_block=context_block,
        )
    except ChatTimeoutError as exc:
        await websocket.send_json({"type": "system_error", "status": 503, "error": str(exc), "message": str(exc)})
        return
    await stream_text(websocket, response)


async def generate_chat_response(
    message: str,
    history: list[dict],
    state: dict,
    *,
    chat_system_prompt: str,
    context_block: str,
) -> str:
    metrics = state.get("metrics") or {}
    anomalies = state.get("anomalies") or []
    predictions = state.get("predictions") or []
    optimizations = state.get("optimizations") or []
    thermal = state.get("thermal") or {}
    scheduler = state.get("scheduler") or {}
    top_processes = metrics.get("top_processes") or []
    recent_actions = state.get("actions") or []
    similar_events = state.get("similar_events") or []
    machine_name = (state.get("machine") or {}).get("name", "this machine")

    if settings.anthropic_api_key and Anthropic:
        client = Anthropic(api_key=settings.anthropic_api_key)
        claude_messages: list[dict[str, str]] = []
        for item in history[-12:]:
            content = str(item.get("content") or "").strip()
            if not content:
                continue
            role = "assistant" if str(item.get("role")) == "assistant" else "user"
            claude_messages.append({"role": role, "content": content})
        claude_messages.append({"role": "user", "content": f"{context_block}\n\nUser message:\n{message}"})

        try:
            completion = await call_claude(
                client=client,
                system_prompt=chat_system_prompt,
                messages=claude_messages,
                model="claude-sonnet-4-20250514",
                max_tokens=600,
            )
        except asyncio.TimeoutError as exc:
            raise ChatTimeoutError("AI response timed out. Try again.") from exc
        except Exception as exc:  # pragma: no cover
            logger.error("PulseAI request failed: %s", exc)
            return "PulseAI could not reach Claude right now. Please retry shortly."

        content = completion.content[0].text if completion.content else ""
        return content or "I could not generate a response."

    lower_message = message.lower()
    if "cpu" in lower_message:
        offender = top_processes[0] if top_processes else None
        if offender:
            return (
                f"{machine_name} is at {metrics.get('cpu_total', 0):.1f}% CPU right now. "
                f"The top active process is {offender['name']} (PID {offender['pid']}) at "
                f"{offender['cpu_percent']:.1f}% CPU. "
                f"{'PulseBoost has an optimization queued for CPU waste.' if any(item['pillar'] == 'cpu' for item in optimizations) else 'Current CPU load is within the learned pattern.'}"
            )
    if "ram" in lower_message or "memory" in lower_message:
        ram = metrics.get("ram_percent", 0.0)
        ram_prediction = next((item for item in predictions if item["metric"] == "ram_percent"), None)
        text = f"RAM usage is {ram:.1f}%."
        if ram_prediction:
            text += f" Forecast: {ram_prediction['time_to_threshold_human']} to {ram_prediction['threshold']}% at {ram_prediction['confidence']:.1f}% confidence."
        return text
    if "benchmark" in lower_message or "proof" in lower_message:
        benchmark_history = state.get("benchmark_results") or []
        if benchmark_history:
            latest = benchmark_history[0]
            return (
                f"The latest benchmark for {latest['workload_name']} returned {latest['verdict']}. "
                f"CPU delta was {latest.get('cpu_delta')} and unsupported metrics were reported honestly where frame hooks were unavailable."
            )
        return "No benchmark results have been recorded yet. Run Benchmark Mode to capture before/after evidence for a tweak set."
    if "20 minutes" in lower_message or "history" in lower_message:
        timeline = state.get("timeline", [])
        if timeline:
            point = timeline[max(0, len(timeline) - min(len(timeline), 20))]
            return (
                f"Around that earlier point, health was {point['health_score']:.1f} with CPU {point['cpu_total']:.1f}% "
                f"and RAM {point['ram_percent']:.1f}% during a {point['session_mode']} session."
            )
    if "process" in lower_message and top_processes:
        offender = top_processes[0]
        return (
            f"The heaviest process is {offender['name']} (PID {offender['pid']}) using "
            f"{offender['cpu_percent']:.1f}% CPU and {offender['memory_percent']:.1f}% RAM. "
            f"{'PulseBoost has a live optimization available.' if optimizations else 'No remediation is queued yet.'}"
        )
    if "restart" in lower_message:
        return (
            "I would not recommend restarting yet unless pressure keeps rising or the machine becomes unresponsive. "
            f"Current health is {state.get('health_score', 0):.1f} with {len(anomalies)} active anomalies."
        )
    if "optimize" in lower_message or "efficiency" in lower_message:
        best = optimizations[0] if optimizations else None
        if best:
            return (
                f"Efficiency is {state.get('efficiency_score', 100):.1f}. "
                f"The best live optimization is {best['title'].lower()}: {best['estimated_gain']}. "
                f"PulseBoost marked it as {best.get('status', 'pending')}."
            )
        return f"Efficiency is {state.get('efficiency_score', 100):.1f} and I do not see a better safe optimization than staying as-is right now."
    if "thermal" in lower_message or "temperature" in lower_message:
        return thermal.get("user_message") or "Thermal telemetry is not available right now."
    if "schedule" in lower_message or "maintenance" in lower_message:
        return (
            f"PulseBoost's best maintenance window is {scheduler.get('optimal_maintenance_window', '23:00')}. "
            f"There are {len(scheduler.get('queued_tasks', []))} queued deferred tasks."
        )
    if "similar" in lower_message or "before" in lower_message:
        if similar_events:
            event = similar_events[0]
            return (
                f"I found a similar prior event: {event['summary']}. "
                f"It changed health by {event['score_delta']:.1f} and is the closest memory match PulseBoost has stored."
            )
    return (
        f"Health is {state.get('health_score', 0):.1f} and efficiency is {state.get('efficiency_score', 100):.1f} "
        f"in {state.get('session_mode', 'normal')} mode. There are {len(optimizations)} live optimization opportunities, "
        f"{len(predictions)} predictive signals, and {len(recent_actions)} recent logged actions. "
        f"Ask what to optimize, why a process is wasteful, or what PulseBoost already changed."
    )


async def stream_text(websocket: WebSocket, text: str) -> None:
    for token in text.split():
        await websocket.send_json({"type": "chat_token", "token": token + " "})
        await asyncio.sleep(0.01)
    await websocket.send_json({"type": "chat_done"})


def _format_gb(value_bytes: float | int | None) -> str:
    if value_bytes is None:
        return "unknown"
    return f"{(float(value_bytes) / (1024 ** 3)):.1f}"


def _format_float(value: float | int | None, fallback: str = "unknown") -> str:
    if value is None:
        return fallback
    return f"{float(value):.1f}"


async def build_chat_context_block(websocket: WebSocket, state: dict) -> str:
    foundation = getattr(websocket.app.state, "foundation", {}) or {}
    profile = foundation.get("hardware_profile") or {}
    metrics = state.get("metrics") or {}
    active_session = state.get("active_session") or {}

    gpu_name = profile.get("gpu_model") or "unknown"
    gpu_temp = "unknown"
    gpu_vram_used = "unknown"
    gpu_vram_total = "unknown"
    try:
        gpu_stats = await websocket.app.state.gpu_controller.stats()
        if gpu_stats.get("model"):
            gpu_name = gpu_stats.get("model")
        if gpu_stats.get("temperature_c") is not None:
            gpu_temp = _format_float(gpu_stats.get("temperature_c"))
        if gpu_stats.get("memory_used_mb") is not None:
            gpu_vram_used = f"{(float(gpu_stats.get('memory_used_mb')) / 1024):.1f}"
        if gpu_stats.get("memory_total_mb") is not None:
            gpu_vram_total = f"{(float(gpu_stats.get('memory_total_mb')) / 1024):.1f}"
    except Exception:  # pragma: no cover
        pass

    cpu_name = profile.get("cpu_name") or "unknown"
    cpu_cores = profile.get("cpu_physical_cores") or profile.get("cpu_logical_cores") or "unknown"
    cpu_load = _format_float(metrics.get("cpu_total"))

    ram_total = _format_gb(profile.get("ram_total_bytes") or metrics.get("ram_total"))
    ram_used = _format_gb(metrics.get("ram_used"))
    ram_percent = _format_float(metrics.get("ram_percent"))

    active_game = active_session.get("game_name") or "none"
    game_profile = active_session.get("profile_name") or state.get("session_mode", "normal")
    opt_profile = state.get("session_mode", "normal")
    health_score = _format_float(state.get("health_score"), fallback="0.0")
    windows_version = profile.get("os_version") or "unknown"
    build_number = profile.get("os_build") or "unknown"
    recent_tweaks = ", ".join(
        str(item.get("action_type") or item.get("action") or "unknown")
        for item in (state.get("actions") or [])[:5]
    ) or "none"

    return (
        "You have real-time access to this user's system state (injected as context before each message):\n"
        f"- CPU: {cpu_name}, {cpu_cores} cores, current load {cpu_load}%\n"
        f"- RAM: {ram_total}GB total, {ram_used}GB used ({ram_percent}%)\n"
        f"- GPU: {gpu_name}, {gpu_temp}C, VRAM {gpu_vram_used}/{gpu_vram_total}GB\n"
        f"- Active game: {active_game} (profile: {game_profile})\n"
        f"- Current optimization profile: {opt_profile}\n"
        f"- Health score: {health_score}/100\n"
        f"- OS: Windows {windows_version}, build {build_number}\n"
        f"- Recent tweaks applied: {recent_tweaks}"
    )

