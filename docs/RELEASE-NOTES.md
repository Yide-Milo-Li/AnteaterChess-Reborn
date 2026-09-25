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

Breaking changes: all former public headers, C APIs and ABI are removed. Consult [Migration](MIGRATION.md). No new network mode, game-saving feature, or gameplay rule is introduced. Experimental uses Hard. Candidate capacity is 1,024 with sticky failure enforcement and chain length ten. macOS is outside first-release support. Validation evidence is tracked in [VALIDATION.md](VALIDATION.md); no unmeasured performance gain is claimed.
