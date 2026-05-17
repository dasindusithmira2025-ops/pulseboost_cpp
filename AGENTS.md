# AGENTS.md

## Mission
You are building and finishing **PulseBoost**, a premium Windows gaming and PC optimization desktop application.

Your job is to **execute**, not brainstorm endlessly. Improve the existing codebase in place, phase by phase, until the product is stable, auditable, safe, polished, and demonstrably useful.

## Permanent repo rules
1. **Do not change the core stack**
   - Backend: Python
   - API: FastAPI
   - Frontend: Electron + React + Vite + Tailwind CSS
   - Storage: SQLite as primary app database
   - IPC: localhost HTTP and/or safe IPC bridge

2. **Modify the existing codebase in place**
   - Do not create a parallel rewrite.
   - Do not abandon working entry points unless absolutely necessary.
   - Refactor incrementally and preserve working behavior where possible.

3. **No placeholder implementations**
   - No fake success states.
   - No TODO-only stubs in shipped code.
   - No misleading UI controls.

4. **Every system change must be transparent**
   - Save previous state before applying any tweak.
   - Log all changes to the audit log.
   - Store before/after values where applicable.
   - Explain each change in plain English.
   - Provide revert support for every applicable action.

5. **Safety comes before aggressiveness**
   - Never apply dangerous or destructive changes silently.
   - Never suspend or deprioritize critical processes.
   - Unsupported features must surface as unavailable, not silently fail.
   - Prefer safe fallback behavior over risky behavior.

6. **Windows-specific code must be isolated**
   - Wrap registry/service/WMI/NVAPI/PowerShell/netsh/sc.exe usage behind abstractions.
   - Keep non-Windows testability via mocks/fakes.

7. **Testing is mandatory**
   - Add or update tests for every major module.
   - Prefer meaningful unit tests and integration tests.
   - Keep the project runnable after each phase.

8. **Documentation is mandatory**
   - Update `PROGRESS.md` after each completed phase.
   - Update architecture docs when major structure changes.
   - Record risks, tradeoffs, and blocked items honestly.

## Product standards
PulseBoost must feel like:
- premium desktop software
- advanced performance utility
- trustworthy system tool
- data-dense observability dashboard
- safe power-user product

The product must emphasize:
- transparency
- auditability
- revertibility
- measurable impact
- live metrics
- per-session awareness
- hardware-aware behavior

## Execution behavior
- Read `docs/PULSEBOOST_SPEC.md`, `PLANS.md`, `implement.md`, and `PROGRESS.md` before making major changes.
- Execute phase by phase.
- After each phase:
  - summarize what changed,
  - list files changed,
  - run tests/checks,
  - list risks/tradeoffs,
  - update `PROGRESS.md`,
  - continue unless truly blocked.

## Code quality expectations
- full imports
- full type hints
- robust error handling
- structured logging
- clear service/repository boundaries
- maintainable module boundaries
- honest unsupported states
- no silent failures

## Safety guardrails
Critical processes that must never be suspended or harmed include at minimum:
- MsMpEng.exe
- audiodg.exe
- nvcontainer.exe
- dwm.exe
- csrss.exe
- lsass.exe

Service handling must be non-destructive.
PowerShell policy changes must be Process-scoped only.
Risky GPU or power changes must be gated behind explicit confirmation/expert mode.

## Final goal
PulseBoost should ship as a polished, trustworthy, AAA-feeling Windows optimization suite with:
- a strong foundation,
- a clear audit trail,
- safe rollback,
- real-time visibility,
- benchmark/proof features,
- premium frontend polish.