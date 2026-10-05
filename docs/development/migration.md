# Migration from AnteaterChess

Baseline: `fb6df82eba4d513bbc160d2d848ffe6abc41cd3e` on original `main`, 215 commits. The independent Reborn repository retains those commits. The original repository remains the `upstream` reference and receives no refactor pushes.

| Old path/interface | Reborn replacement |
| --- | --- |
| `include/core`, `src/core` | `include/anteater/types.h`, `src/rules`, `src/policy/config.cpp` |
| `src/gameplay`, `src/input` | `src/rules` generation, request parsing, resolution, validation and execution |
| `GameState` | compact `AcPosition` plus opaque `AcSession`; Qt models own copied projections |
| `applyMove` / `undoMove` | `ac_position_apply` / `ac_position_unmake` for rules; `ac_session_submit` / `ac_session_undo` for games |
| `Controller`, FSM, event queue | synchronous `AcSession` commands; desktop-owned navigation and snapshot QThread scheduling |
| `src/turn`, `src/time` | injected `AcClock`, session tick/turn state |
| `src/log` / `bin/logs` | `apps/qt/runtime/runtime.cpp`; existing logs are not migrated |
| `src/ai/ai.c` | `src/ai/{context,search,evaluation,ordering,table,budget}.c` |
| external Experimental plugin probe | removed; old difficulty value 4 is invalid |
| `src/ui`, `src/main.c` | `apps/qt` |
| runtime relative asset files | Qt resources in `assets/resources.qrc` and embedded QML |
| `doc/*.pdf` | identical bytes in `docs/legacy/*.pdf` |
| `packaging/` | `tools/packaging/templates/`, packaging and verification scripts |
| root `tests/test_*.c` | categorized `tests/{rules,session,ai,qt}` |

The old C API, ABI and header paths are intentionally unsupported. `tools/legacy/migration-symbols.json` records mechanical names used during migration; removed state/controller symbols in that inventory are historical, not exported compatibility aliases. There is no saved-game format to migrate.

Rules retain the original variant and limitations documented in the [current manual](../user/manual.md). New hash keys are deterministic but not numerically compatible with old keys. Revision-based task validation replaces the old move-count/turn checks. Logging errors are separate diagnostics. Documentation correcting the historical PDFs includes the 10-file board, Anteater restrictions, timer skip policy, AI-only repetition and four promotion types.

## Reborn layout migration

The historical GTK sources were categorized under `apps/gtk/{app,screens,ui,async}`.
The current desktop is `apps/qt/{app,models,async,qml,runtime}`. The former
platform runtime is replaced by Qt executable-path, clock and logging services.
Assets remain under `pieces/` and `icons/`; `WhiteAntsvg.svg` is physically renamed
to `WhiteAnt.svg` while its Qt resource alias stays compatible.
Current documents move under `docs/{user,architecture,development}`; historical
PDFs remain under `docs/legacy` with identical bytes. Tool paths are now
`tools/{packaging,dev,legacy}`; direct script invocations use these locations.
Make command names and the core public C interfaces are retained through the
CMake/Ninja forwarding wrapper.

New game logs now live in `logs/` beside the actual executable on both supported
platforms. Earlier user-data/state logs remain in their original directories.

## Qt desktop cutover

The 8fc6f7d GTK desktop is available through Git history; it is not copied into a
source archive. CMake and Ninja own the build graph and Make forwards commands.
Qt models copy borrowed history immediately. Workers copy position/hash history,
use their own search context and atomically cancel; live Session access stays on
the main thread. QML embeds the existing SVG files/aliases and retains left-select,
right-move, typed fields, promotion/cancel, F11/Escape and game confirmation flows.

Experimental was a Hard alias. It has been removed from current enums, branches,
menus and help. Difficulty values remain None=0, Easy=1, Medium=2, Hard=3,
Tournament=5. Session explicitly rejects 4 rather than remapping it.

The retired runtime's Cairo internal-function scan is deleted. With the locally
installed Cairo 1.18.6-2, isolated baseline smoke tests succeeded both with and
without the patch; the original rendering failure was not reproduced. This is not
proof of an upstream Cairo fix. The new Windows path uses Qt SVG and rejects
retired runtime modules. Package dependency records disclose indirect GLib usage.
See [migration validation](qt-migration-validation.md) for host checks and limits.
