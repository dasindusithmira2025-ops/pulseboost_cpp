# Known Limitations

## V1 Runtime Boundaries
- Electron is now the preferred desktop shell path when installed.
- PyWebView remains as the fallback launcher and compatibility path.
- Installer packaging for the Electron shell is still not implemented in this repo.

## Benchmarking
- Live FPS, 1% low FPS, and frame-time values are available only when `PRESENTMON_CSV_PATH` points to a live PresentMon-compatible CSV source.
- Without that trusted frame-time source, frame capture reports an explicit unavailable state instead of fabricating FPS.
- Benchmark FPS/1% low/frame-time evidence still requires integration with the same trusted frame-time source.
- Benchmark evidence is strongest today on CPU and, when diagnostics succeed, network deltas.
- GPU deltas only appear when supported telemetry is available on the current machine.

## Network
- Public latency and jitter are derived from TCP connect timing, not raw ICMP.
- Router and active game-server probes remain unavailable until a safe gateway/server discovery path exists.
- QoS changes are preview-only and remain dry-run.

## GPU
- Live GPU telemetry depends on a supported vendor runtime and is often unavailable on the current machine.
- GPU setting writes remain dry-run.
- BIOS guidance is advisory-only and does not pretend to read unavailable firmware state.

## Game Profiles
- Detected games are sourced from observed sessions and benchmark history, not a full installed-games scanner.
- Profile import/export uses JSON `.pbprofile` payloads and intentionally avoids executing arbitrary external logic.

## Validation / Packaging
- The frontend bundle still triggers Vite's large-chunk warning.
- There is no browser-automation suite yet; frontend validation is currently build-based.
- Installer packaging and uninstaller automation are documented but not implemented in this repo.
