# Qt migration candidate (unpublished)

C11 core retained, with a C++17 Qt 6/QML desktop and one CMake/Ninja graph behind
Make commands. Models copy Session data; snapshot workers use atomic cancellation
and stale-result/close guards. Existing SVG/dark theme, controls and variant policies
remain. Experimental is removed; value 4 is invalid and Tournament remains 5.
The GTK desktop, GLib adapters, GResource build and Cairo internal-function scan
are retired. AI evaluation is split without tuning weights/search algorithms.

See [validation](qt-migration-validation.md) for exact local coverage and external
acceptance still required. Candidate archives are not a published release.

# Unreleased layout and desktop runtime changes

- Organize GTK sources by application, screens, UI and background tasks; separate
  platform runtime and logging responsibilities. Categorize resources, documents
  and tools while retaining core interfaces and embedded resource aliases.
- Windows game builds use the GUI subsystem by default. `WINDOWS_CONSOLE=1`
  selects a separate console build for debugging; tests remain console programs.
- Both platforms store game snapshots in `logs/` beside the actual executable.
  Old user-directory logs remain in place. Log failures remain diagnostics after
  accepted game actions.
- Extend adapter/resource tests and host-package verification to cover all assets,
  Unicode/space paths, concurrent sessions, symlinks, subsystem flags and blocked
  log directories. The existing release version is retained for these local builds.

# 2.0.1 — UI Modernization & Fullscreen Typography

This release introduces an enhanced, modern visual design and typography refinement while preserving all variant rules, C11 architectures, and performance guarantees:

- **Obsidian Dark Theme**: Modernized styling with dark slate/amber palette, refined button states, smooth borders, and high-contrast status feedback.
- **Dynamic HiDPI Chessboard & SVG Rendering**: Real-time vector scaling for pieces and boards, ensuring crisp display on high-resolution monitors.
- **Clean User Interface Copy**: Removed internal "Ac" prefixes from all user-facing labels, placeholders, popovers, move history entries, setup dialogs, and error messages.
- **Adaptive Fullscreen Typography**: Automatically scales move history font size to 16px and balances sidebar proportions when entering fullscreen mode.
- **Robustness & Fuzzing Audit**: Added extensive parser fuzzing, MoveList boundary audits, and transactional rollback tests in the core rules engine.

# 2.0.0 — AnteaterChess Reborn

This major version replaces the internal API and repository layout while preserving the local GTK game and variant rules. Compact reversible positions replace history-heavy game-state copies. Opaque sessions own histories, clocks, results and diagnostics. Independent search contexts replace mutable process-level search caches. GTask jobs own cancellation and snapshots until worker completion, and resources are embedded.

GNU Make builds isolated module libraries and platform/configuration outputs. Current English documentation, agent instructions, baseline comparisons, injected-clock tests, desktop automation, sanitizer CI and source/runtime packaging accompany the refactor. Windows ships a portable dependency bundle; Ubuntu uses system GTK 3. The original 215 commits, authorship, COPYRIGHT and course PDFs remain intact.

Breaking changes: all former public headers, C APIs and ABI are removed. Consult [Migration](migration.md). No new network mode, game-saving feature, or gameplay rule is introduced. Experimental uses Hard. Candidate capacity is 1,024 with sticky failure enforcement and chain length ten. macOS is outside first-release support. Validation evidence is tracked in [VALIDATION.md](validation.md); no unmeasured performance gain is claimed.
