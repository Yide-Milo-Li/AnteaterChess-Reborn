# Validation record

Baseline reference: original commit `fb6df82eba4d513bbc160d2d848ffe6abc41cd3e`.

Windows local UCRT64: core tests, GUI compilation and GTK handler-driven integration tests passed. Initial variant perft depths 1–3 are **24, 576, 15,286**, generated from the original engine; the new engine matches these counts, a fixed 50-ply legal-move fingerprint sequence, and five focused fixture boards. Random 150-ply apply/unmake checks restore the exact original compact position and compare incremental/full hashes.

Session tests cover isolated clocks/log callbacks, partial allocation failure cleanup, failed log diagnostics after accepted moves, turn timeout, undo, stale AI results, AI-vs-AI repetition and Tournament expiry. Search tests cover independent contexts, unchanged input positions, cancellation, budget exhaustion and Tournament budget arithmetic. GTK automation covers mode navigation, SVG resource loading, typed moves, undo, cancelled hints, AI turns in both AI modes, and closing during search.

Windows portable-package verification passed with MSYS2 removed from PATH, a non-default working directory, and an extraction path containing spaces and Chinese characters. Source archives rebuilt the core without GTK parameters, repackaged successfully, and regenerated after a temporary README change. Original PDF bytes were retained (Git records 100% identical moves).

The first Ubuntu 24.04 CI job passed core-without-GTK, Release desktop/Xvfb, ASan/UBSan, runtime packaging and extracted-source rebuild checks in [run 36089996640](https://github.com/Yide-Milo-Li/AnteaterChess-Reborn/actions/runs/36089996640). The release gate requires both matrix jobs to pass again on the exact published commit; its run is linked from the Release.

Local Windows x64 GCC 16.1.0, `CONFIG=release`, initial position, depth 3: 1,592 nodes, 5 ms in one observed run, 39,134,480 bytes of explicitly allocated search-context/table/workspace memory, and a 672-byte position. This is allocated workspace, not measured process peak RSS; GTK, allocator metadata and operating-system overhead are excluded. Run `make CONFIG=release benchmark` to reproduce; CI records platform-specific output. No speedup relative to the original engine is claimed.

Additional regression coverage includes both-color special-move round trips, AI check evasion/terminal/promotion/en-passant positions, full 1,024-move session capacity, human-color undo policies, completed hints, and rapid game replacement. Automated GTK operation is not exhaustive human visual review; interactive promotion-dialog layout and every timer/endgame display variant have not received a manual acceptance pass. macOS and platforms outside the release matrix are unverified.
