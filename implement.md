# PulseBoost Implementation Runbook

This file defines how Codex should behave while executing the project.

## Core behavior
You are not here to generate abstract ideas. You are here to implement the product in the existing repository.

You must:
- inspect before changing
- preserve working code where possible
- refactor incrementally
- keep the project runnable
- avoid parallel rewrites
- choose safe fallbacks when OS/runtime limitations exist

## Mandatory working style
1. Read:
   - `AGENTS.md`
   - `docs/PULSEBOOST_SPEC.md`
   - `PLANS.md`
   - `PROGRESS.md`

2. Start from the current phase in `PROGRESS.md`.

3. Before major edits:
   - inspect the current repo structure
   - identify existing files that already partly solve the task
   - decide whether to extend, refactor, or replace minimally

4. After each phase:
   - summarize work completed
   - list files created/updated
   - run tests/checks
   - list blockers/tradeoffs
   - update `PROGRESS.md`
   - continue automatically

## Architecture rules
- Keep Windows-specific logic isolated behind wrappers/adapters.
- Keep vendor SDK logic isolated behind interfaces.
- Keep persistence logic separate from core domain services.
- Keep API routers thin; business logic belongs in services/core modules.
- Keep frontend API access typed and centralized.
- Prefer composition over giant god classes.

## Safety execution rules
- Never apply a tweak without saving original state first.
- Never silently apply registry/service/GPU/network changes.
- Never fake successful hardware control.
- Never suspend critical processes.
- Never leave temporary session tweaks active after unclean exit without recovery logic.
- Never expose risky controls without warning/confirmation path.

## Unsupported capability behavior
When a runtime dependency or OS capability is unavailable:
- detect it clearly
- record capability state
- expose unsupported status in API/UI
- skip the action honestly
- do not fake success

## Testing behavior
- Add/update tests continuously.
- Prefer unit tests for logic and integration tests for persistence/API flows.
- Use mocks/fakes for Windows-only code paths when real execution is unavailable.
- Keep tests meaningful.

## Documentation behavior
- Update `PROGRESS.md` after each phase.
- Update architecture notes when structure changes materially.
- Record important decisions and tradeoffs honestly.

## Output behavior after each phase
Provide:
- phase summary
- changed files
- tests/checks run
- blockers or risks
- next phase start

Then continue unless truly blocked.

## Blocker policy
Only stop and ask for input if blocked by something such as:
- missing credentials
- missing proprietary SDK required for a real integration
- missing operating system capability that cannot be reasonably abstracted
- severe ambiguity that risks damaging the repo

Otherwise choose the safest reasonable implementation path and continue.

## Quality bar
This product must feel like:
- premium desktop software
- technically credible
- safe for power users
- professionally engineered
- measurable and trustworthy

Do not optimize for speed at the cost of integrity.