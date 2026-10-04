# Qt desktop migration and maintenance reduction

Baseline: `8fc6f7d`. Approved scope: C11 rules/session/AI retained, Qt 6/QML
desktop with C++17 adapters, one CMake graph with Make command wrappers,
Experimental removed, existing visual language and interaction retained.

## Gates

| Stage | Deliverable | Status |
| --- | --- | --- |
| 0 | Fixed baseline, behavior matrix, AI reference, Cairo comparison | Complete locally |
| 1 | Independent C core, C++ linkage, Qt/resources on both hosts | Complete locally |
| 2 | Session/models, monotonic clocks, executable-relative logs | Complete locally |
| 3 | Snapshot worker, cooperative cancellation, stale-result guards | Complete locally |
| 4 | Complete QML pages, four difficulties, interaction checks | Complete locally |
| 5 | Portable packages, source rebuild, removal of GTK/Cairo | Complete locally; external release review pending |
| 6 | Evaluation responsibilities extracted without behavior changes | Complete locally |

The GTK implementation is now a Git-history reference only; Qt automated and
rendered checks passed before its source and build graph were removed.
Do not adjust AI weights, ordering, SEE, pruning, rules, undo, Tournament budgets,
or AI-only repetition adjudication. Experimental's old value 4 is invalid;
Tournament retains value 5. Historical PDFs, COPYRIGHT, and attribution retain
their original bytes. New documentation is English.

## Architecture

The main-thread session adapter owns AcSession. Board/history models copy
borrowed snapshot data immediately. QML owns presentation and navigation.
Workers own positions, hash history, options, search contexts and cancellation
flags until exit. Results require matching page, revision and generation.
Runtime services supply injected monotonic time and diagnostic logging.
Closing cancels work and joins it before releasing the session.

## Acceptance matrix

| Behavior | Required evidence |
| --- | --- |
| Rules/session | Existing fixtures, hashes, special moves, failures, clocks/undo |
| Difficulty | Four visible levels; 4 rejected; Tournament stays 5 |
| Search lifecycle | Replacement, undo, timeout, navigation, hint cancellation, close |
| Interaction | Coordinates, left/right click, promotion/cancel, focus, history, F11/Escape |
| Rendering | 14 pieces, 4 icons, 32/96/160 sizes, fractional DPI, software/default backend |
| Runtime | Unicode/spaces, other cwd, concurrent logs, blocked paths, Linux symlink |
| Distribution | Clean PATH, no development runtime, source rebuild without Git, PDF hashes |
| AI extraction | Fixed reference scores/SEE/search move/depth/nodes; sanitizers |

Each stage is recorded separately. Automated display tests and agent-rendered
inspection must be distinguished from human native-desktop acceptance. Candidate
archives are local outputs; publishing and remote CI results are separate.
