# Repository instructions

Read [the specification](docs/Chess_SoftwareSpec.md) before changing module boundaries, ownership, error handling, clocks, or background jobs. Read [the user manual](docs/Chess_UserManual.md) before changing rules or interaction. For build, test, packaging, and release work use [DEVELOPMENT.md](docs/DEVELOPMENT.md). For old symbols or paths use [MIGRATION.md](docs/MIGRATION.md).

- Retain C11, GTK 3, and GNU Make. Core modules must compile without GTK or GLib.
- Route desktop game commands through `AcSession`; use the rules module for both GUI legality and AI moves. Page navigation belongs to GTK.
- Preserve this variant's rules, timeout/undo policies, Tournament budgets, and AI-only automatic repetition draw. A rule change requires explicit product scope and regression fixtures.
- Keep mutable search data in its owning `AcSearchContext`. Background jobs own snapshots and cancellation data until the worker exits. Validate session revision and GUI generation before accepting a result.
- Keep operations transactional. Report log failures as diagnostics after an accepted move. Propagate move-buffer overflow explicitly.
- For logic changes run `make test`; for desktop/resource changes also run `make test-gui`; for packaging/docs run `make check` and `make CONFIG=release package-source package`, then `python3 tools/verify.py`. Record actual validation and limitations in the change description.
- Preserve COPYRIGHT, team attribution, and the bytes of historical PDFs. Write new documentation in English. Generated files belong under `build/` or `dist/`.
