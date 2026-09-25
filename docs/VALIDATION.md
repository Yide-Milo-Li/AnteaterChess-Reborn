# Validation record

Baseline reference: original commit `fb6df82eba4d513bbc160d2d848ffe6abc41cd3e`.

Windows local UCRT64: core tests, GUI compilation and GTK handler-driven integration tests passed. Initial variant perft depths 1–3 are **24, 576, 15,286**, generated from the original engine; the new engine matches these counts, a fixed 50-ply legal-move fingerprint sequence, and five focused fixture boards. Random 150-ply apply/unmake checks restore the exact original compact position and compare incremental/full hashes.

Session tests cover isolated clocks/log callbacks, partial allocation failure cleanup, failed log diagnostics after accepted moves, turn timeout, undo, stale AI results, AI-vs-AI repetition and Tournament expiry. Search tests cover independent contexts, unchanged input positions, cancellation, budget exhaustion and Tournament budget arithmetic. GTK automation covers mode navigation, SVG resource loading, typed moves, undo, cancelled hints, AI turns in both AI modes, and closing during search.

CI and release verification results will be recorded when the exact release commit is validated. Automated GTK operation is not a claim of exhaustive human visual review. macOS and platforms other than the release matrix are unverified. Performance reports record measurements rather than promising speedups.
