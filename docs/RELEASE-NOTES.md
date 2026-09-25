# 2.0.0 — AnteaterChess Reborn

This major version replaces the internal API and repository layout while preserving the local GTK game and variant rules. Compact reversible positions replace history-heavy game-state copies. Opaque sessions own histories, clocks, results and diagnostics. Independent search contexts replace mutable process-level search caches. GTask jobs own cancellation and snapshots until worker completion, and resources are embedded.

GNU Make builds isolated module libraries and platform/configuration outputs. Current English documentation, agent instructions, baseline comparisons, injected-clock tests, desktop automation, sanitizer CI and source/runtime packaging accompany the refactor. Windows ships a portable dependency bundle; Ubuntu uses system GTK 3. The original 215 commits, authorship, COPYRIGHT and course PDFs remain intact.

Breaking changes: all former public headers, C APIs and ABI are removed. Consult [Migration](MIGRATION.md). No new network mode, game-saving feature, or gameplay rule is introduced. Experimental uses Hard. Candidate capacity is 1,024 with sticky failure enforcement and chain length ten. macOS is outside first-release support. Validation evidence is tracked in [VALIDATION.md](VALIDATION.md); no unmeasured performance gain is claimed.
