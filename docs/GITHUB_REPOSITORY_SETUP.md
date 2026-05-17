# GitHub Repository Setup

This repository is configured as the official source tree for PulseBoost.

## Included Repository Infrastructure

- GitHub Actions CI for backend compile checks, Python tests, frontend build, and desktop entry-point validation.
- Dependabot update checks for npm, pip, and GitHub Actions dependencies.
- Issue templates for bugs and safe feature proposals.
- Pull request template with validation and safety checklist.
- Security policy covering system-utility risk expectations.
- Contribution guide for stack, testing, documentation, and rollback rules.
- Root `.gitignore`, `.gitattributes`, and `.editorconfig` for source-first repository hygiene.

## Recommended GitHub Settings

Configure these in the repository settings after the initial push:

- Default branch: `main`
- Branch protection for `main`
- Require CI checks before merge
- Require pull request review before merge
- Enable Dependabot alerts and security updates
- Enable private vulnerability reporting
- Add repository description: `Premium Windows gaming and PC optimization desktop application`
- Add topics: `windows`, `electron`, `react`, `fastapi`, `sqlite`, `gaming`, `optimization`

## Source Hygiene

Do not commit:

- `node_modules`
- portable Python or Node runtime folders
- runtime SQLite databases
- logs
- `.env` files
- generated installers or archives
- local app data under `data/` or `pulseboost/data/`

The source tree should stay reproducible from manifests, scripts, and documentation.
