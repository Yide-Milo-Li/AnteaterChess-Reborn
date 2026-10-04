# Repository instructions

Read [the specification](docs/architecture/software-specification.md) before changing module boundaries, ownership, error handling, clocks, or background jobs. Read [the user manual](docs/user/manual.md) before changing rules or interaction. For build, test, packaging, and release work use [DEVELOPMENT.md](docs/development/guide.md). For old symbols or paths use [MIGRATION.md](docs/development/migration.md).

- Retain C11 core, C++17 Qt 6/QML desktop, and the single CMake graph. GNU Make forwards commands. Core modules must compile without Qt, GTK or GLib.
- Route desktop game commands through `AcSession`; use the rules module for both GUI legality and AI moves. Page navigation belongs to the desktop adapter and QML.
- Preserve this variant's rules, timeout/undo policies, Tournament budgets, and AI-only automatic repetition draw. A rule change requires explicit product scope and regression fixtures.
- Keep mutable search data in its owning `AcSearchContext`. Background jobs own snapshots and cancellation data until the worker exits. Validate session revision and desktop generation before accepting a result.
- Keep operations transactional. Report log failures as diagnostics after an accepted move. Propagate move-buffer overflow explicitly.
- For logic changes run `make test`; for desktop/resource changes also run `make test-gui`; for packaging/docs run `make check` and `make CONFIG=release package-source package`, then `python3 tools/packaging/verify.py`. Record actual validation and limitations in the change description.
- Preserve COPYRIGHT, team attribution, and the bytes of historical PDFs. Write new documentation in English. Generated files belong under `build/` or `dist/`.
