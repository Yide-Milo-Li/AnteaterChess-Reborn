# Software specification

## Architecture

Public interfaces: [types](../../include/anteater/types.h), [rules](../../include/anteater/rules.h), [session](../../include/anteater/session.h), [AI](../../include/anteater/ai.h). All public library symbols use `ac_`, `Ac`, or `AC_`. Qt desktop internals are private to the application.

```mermaid
flowchart TD
    QML[QML pages and input] --> Adapter[Session adapter commands]
    Adapter --> Session[AcSession]
    Adapter --> Models[Copied board and history models]
    Models --> QML
    Adapter --> Task[Snapshot QThread worker]
    Task --> AI[AcSearchContext]
    Session --> Rules[Rules and AcPosition]
    AI --> Rules
    Session --> Budget[AI time budget helpers]
    Adapter --> Platform[Qt runtime clock and log adapter]
    Platform --> Session
    Resources[Qt resource system] --> QML
```

An [interactive architecture diagram](architecture.html) is also available.

The session has no GTK or GLib dependency. Platform callbacks are injected. It uses AI budget helpers but never initiates a search. Search consumes copied positions and historical hashes and never mutates a live session. The former Controller/event queue/FSM contracts are replaced by synchronous session commands and application-owned navigation.

## Values, ownership, and lifetime

`AcPosition` contains the 80-square board, current side, four castling-right bits, en-passant Ant square, move count, and deterministic 64-bit hash. It has no UI state or history. `AcMove` records endpoints, original moving piece, special kind, and up to ten capture/path entries. `AcUndo` owns the complete previous compact position, including rights and hash.

`AcMoveList` has 1,024 candidates and a sticky status; overflow is `AC_CAPACITY`, never truncation presented as success. Recursive search workspaces are heap-owned. `ac_position_apply` generates legal candidates, matches the complete request, and only then changes the position; failures preserve it. `ac_position_unmake` restores a trusted undo record. Direct fixture edits must recompute `ac_position_hash` before execution. Low-level board helpers permit composing fixtures, not arbitrary validated game import.

`AcSession` owns heap arrays for 1,024 moves, undo records, and 1,025 hashes. Creation requires a clock callback. Optional allocator/deallocator callbacks must be paired and remain valid until destruction. Allocation failure cleans partial construction. Snapshots copy values but borrow history/hash pointers until the next mutation or destruction; callers must copy them for asynchronous use. Sessions are single-owner objects, not internally locked. Independent sessions may be used independently.

`AcSearchContext` owns its transposition table, killer/history heuristics, per-ply move buffers, undo stack, and hash workspace. One invocation at a time per context. Separate contexts share immutable tables only. Search options and callbacks must remain alive throughout the synchronous search. Destroy the context after it returns.

## Session commands and state

```mermaid
stateDiagram-v2
    [*] --> Idle: create
    Idle --> Active: start
    Active --> Active: accepted move / undo / timeout
    Active --> Finished: mate / draw / Tournament expiry / finish
    Finished --> Active: start
    Active --> Active: start new game
    Finished --> [*]: destroy
    Idle --> [*]: destroy
    Active --> [*]: destroy
```

Start validates configuration and resets history, position, clocks, diagnostics, and Tournament totals. Each accepted mutation increments a revision. Submit and undo first tick the clock; if ticking changed the revision, a human command returns `AC_STALE_RESULT`. AI submission additionally requires the expected revision. Timeout changes the active side and current hash slot without appending history. Repetition adjudication and the 1,024-move capacity policy belong to the session, not rule generation.

Move processing: parse coordinates → resolve canonical special move/path → apply to a temporary position → determine terminal state → append history and undo → publish a new revision → write the diagnostic log. A move's status reports whether the command applied. Log failure is separately available as snapshot `diagnostic`, so callers never retry an already accepted move merely because logging failed.

Timing uses injected monotonic milliseconds. No core test sleeps or busy-waits for a clock. Elapsed game time freezes on finish; turn time resets on move/undo/skip. Tournament totals and saved-time pools are owned by each session and are not refunded by undo. Its arithmetic and limits live in [budget.c](../../src/ai/budget.cpp).

## Rule engine

[movegen.c](../../src/rules/movegen.cpp) generates variant pseudo-legal candidates, then excludes self-check and king captures. [resolver.c](../../src/rules/resolver.cpp) selects explicit promotion variants and rejects multiple non-promotion paths sharing an endpoint. [position.c](../../src/rules/position.cpp) executes and restores compact positions and updates the hash by XORing changed piece/right components. [endgame.c](../../src/rules/endgame.cpp) determines check, no-legal-move outcomes, and the retained material policy. The [manual](../user/manual.md) is the rules reference.

Hashes include pieces, side, castling rights, and a capturable en-passant file. Numeric keys intentionally differ from the old engine. They support repetition and transposition identity; they are not a persistent storage format or cryptographic guarantee.

## Search

[search.c](../../src/ai/search.cpp) retains iterative deepening, aspiration windows,
principal-variation alpha-beta, null-move pruning, late-move reductions and quiescence.
Evaluation is split into material/position tables, heuristic attacks and SEE,
mobility, strategic features and aggregation; each exposes a narrow private header.
[ordering.c](../../src/ai/ordering.cpp) owns killer/history and capture ordering;
[table.c](../../src/ai/table.cpp) owns context-local transpositions.
Experimental has been removed. Valid difficulty values are 0, 1, 2, 3 and 5;
4 is rejected by explicit Session configuration validation. Tournament remains 5.

Results contain the selected legal move, status, completed depth, node count and elapsed milliseconds. Budget exhaustion returns the best available legal move; cooperative cancellation returns `AC_CANCELLED`. Candidate overflow aborts with `AC_CAPACITY`. No legal root move returns `AC_UNAVAILABLE`. Cancellation is checked at search boundaries; elapsed time is checked periodically, so budgets are not hard real-time deadlines.

## Desktop tasks and resources

[search_jobs.cpp](../../apps/qt/async/search_jobs.cpp) owns one job at a time,
with copied position/hash history, search options, atomic cancellation flag,
result, Session revision and desktop generation. A QThread runs the search.
Cancellation sets the atomic flag immediately; it does not require a queued
worker slot. Completion is delivered on the main thread and joined before job
storage is released. Results require matching page, generation and revision,
and an open, uncancelled desktop. Close disables commands, cancels work, then
waits for cooperative exit before freeing Session/log/model storage.

[session_adapter.cpp](../../apps/qt/app/session_adapter.cpp) is the only live
Session owner. It executes commands, ticks every 100 ms using injected monotonic
time, queries Rules for selections and legality, and publishes copied projections.
Board/history models own their values; borrowed snapshot pointers never reach
QML or workers. Getters do not mutate game state or navigation. QML pages own
controls and layouts; a clock tick or ordinary move does not recreate the page.

[resources.qrc](../../assets/resources.qrc) embeds 14 piece and 4 icon SVGs with
stable `/org/anteater/reborn/` aliases. QML is embedded independently. Assets
are independent of cwd. Qt SVG rendering supplies device-scaled images.
[runtime.cpp](../../apps/qt/runtime/runtime.cpp) uses GetModuleFileNameW on
Windows and `/proc/self/exe` on Linux, preserving actual-executable placement
through symlink launches. Path failure produces a diagnostic-only log service.

Logs default to `<executable-directory>/logs/session-<UUID>.log`; an explicitly
injected log directory remains available for fixtures. Each object owns its path
and rotates files when the game ID changes. Directories are created on the first
write. Path discovery failure keeps a diagnostic-only log object: its callback
returns `AC_IO_ERROR` without changing the session command's result. Directory or
file-write failures follow the same contract, with no working-directory or user-data
fallback. A log is a current game snapshot, not a saved-game import format.

Runtime contracts: [QThread](https://doc.qt.io/qt-6/qthread.html), [Qt resources](https://doc.qt.io/qt-6/resources.html), [Windows deployment](https://doc.qt.io/qt-6/windows-deployment.html).

## Error contracts and verification

Commands use `AcStatus`: success, invalid argument, illegal move, allocation failure, capacity, cancellation, stale result, unavailable operation, and external I/O failure. Boolean predicates and selection enums explicitly have their own return conventions. Borrowed pointers are never freed by callers. Core APIs do not terminate the process. Qt's allocation behavior applies inside the desktop adapters. Logs use QSaveFile atomic replacement; failure is diagnostic after accepted operations.

See [test migration](../development/test-migration.md) and [validation](../development/validation.md). Baseline fixtures compare the original move ordering-independent fingerprint, a fixed 50-ply sequence, special boards, and variant perft counts. Random legal apply/unmake checks require byte-identical restoration and full hash recomputation agreement. Session tests cover injected failures, isolation, clocks, repetition and stale results. Qt Test and Quick Test exercise pages, all three modes, hints, undo and shutdown during search. CI supplements these with sanitizers, source rebuilds and runtime smoke tests.
