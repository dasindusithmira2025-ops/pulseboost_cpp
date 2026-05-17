# Release Checklist

## Product Integrity
- [ ] Verify `PROGRESS.md` reflects the final completed state
- [ ] Verify `docs/KNOWN_LIMITATIONS.md` matches the actual runtime behavior
- [ ] Verify `docs/PRODUCTION_READINESS.md` V1 scope and roadmap remain accurate
- [ ] Confirm no UI control claims a live write when the backend is dry-run or unavailable

## Validation
- [ ] Run backend compile checks
- [ ] Run the full Python unit test suite
- [ ] Run the frontend production build
- [ ] Smoke-check the desktop launcher and `/healthz`

## Safety
- [ ] Confirm Trust Center reports admin, recovery, rollback, and safeguard state correctly
- [ ] Confirm protected-process rules remain enforced
- [ ] Confirm expert mode does not bypass unsupported capability guards
- [ ] Confirm temporary tweak cleanup still runs on shutdown and recovery paths

## UX
- [ ] Review loading, empty, and error states on every exposed page
- [ ] Review unavailable/dry-run wording on Benchmark, Network, GPU, Trust Center, and Settings
- [ ] Confirm all documented pages are reachable in navigation
- [ ] Confirm the desktop shell sidebar/top bar/window controls behave correctly in the runtime
- [ ] Confirm `.pbprofile` import/export works end to end

## Packaging / Ship Notes
- [ ] Reconfirm Electron is the preferred runtime and PyWebView fallback still launches cleanly
- [ ] Reconfirm installer work is still roadmap-only and not implied as complete
- [ ] Reconfirm release notes call out dry-run and unavailable hardware surfaces honestly
