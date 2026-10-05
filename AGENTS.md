# Repository instructions

Read [the specification](docs/architecture/software-specification.md) before changing module boundaries, ownership, error handling, clocks, or background jobs. Read [the user manual](docs/user/manual.md) before changing rules or interaction. For build, test, packaging, and release work use [DEVELOPMENT.md](docs/development/guide.md). For old symbols or paths use [MIGRATION.md](docs/development/migration.md).

- Use C++20 and one CMake/CTest/CPack graph. Windows requires native v143 14.44 x64 and official Qt 6.11.2 MSVC 2022 x64; Linux uses GCC and system Qt 6.4.2. Core modules must compile without Qt, GTK or GLib. Core-only builds with tests and development tools disabled must not discover Python.
- Route desktop game commands through `ac::Session`; use the rules module for both GUI legality and AI moves. Page navigation belongs to the desktop controller and QML.
- Preserve this variant's rules, timeout/undo policies, Tournament budgets, and AI-only automatic repetition draw. A rule change requires explicit product scope and regression fixtures.
- Keep mutable search data in its owning `ac::SearchContext`. Background jobs own requests and stop tokens until the worker exits. Validate page, gameId, revision, generation and closing before accepting a result.
- Keep operations transactional. Report log failures as diagnostics after an accepted move. Propagate move-buffer overflow explicitly.
- Configure and build through CMake presets, then run the corresponding CTest preset. For desktop/resource changes include GUI checks; for packaging/docs run development checks and CPack validation. Record actual commands, tested commits, exit codes, environments, attachment hashes and acceptance limitations in docs/development/native-migration/validation.md. Preserve evidence outside disposable build trees.
- Preserve COPYRIGHT, team attribution, and the bytes of historical PDFs. Write new documentation in English. Generated files belong under `build/` or `dist/`.
