# Migration from AnteaterChess

Baseline: `fb6df82eba4d513bbc160d2d848ffe6abc41cd3e` on original `main`, 215 commits. The independent Reborn repository retains those commits. The original repository remains the `upstream` reference and receives no refactor pushes.

| Old path/interface | Reborn replacement |
| --- | --- |
| `include/core`, `src/core` | `include/anteater/types.h`, `src/rules`, `src/session/config.c` |
| `src/gameplay`, `src/input` | `src/rules` generation, request parsing, resolution, validation and execution |
| `GameState` | compact `AcPosition` plus opaque `AcSession`; GTK pages live in private `GuiView` |
| `applyMove` / `undoMove` | `ac_position_apply` / `ac_position_unmake` for rules; `ac_session_submit` / `ac_session_undo` for games |
| `Controller`, FSM, event queue | synchronous `AcSession` commands; GTK-owned navigation and GTask scheduling |
| `src/turn`, `src/time` | injected `AcClock`, session tick/turn state |
| `src/log` / `bin/logs` | `src/platform/runtime.c`, per-user logs; existing logs are not migrated |
| `src/ai/ai.c` | `src/ai/{context,search,evaluation,ordering,table,budget}.c` |
| external Experimental plugin probe | explicit Hard fallback, no plugin lookup |
| `src/ui`, `src/main.c` | `apps/gtk` |
| runtime relative asset files | compiled `assets/resources.xml` GResource |
| `doc/*.pdf` | identical bytes in `docs/legacy/*.pdf` |
| `packaging/` | `tools/templates/`, packaging and verification scripts |
| root `tests/test_*.c` | categorized `tests/{rules,session,ai,gtk}` |

The old C API, ABI and header paths are intentionally unsupported. `tools/migration-symbols.json` records mechanical names used during migration; removed state/controller symbols in that inventory are historical, not exported compatibility aliases. There is no saved-game format to migrate.

Rules retain the original variant and limitations documented in the [current manual](Chess_UserManual.md). New hash keys are deterministic but not numerically compatible with old keys. Revision-based task validation replaces the old move-count/turn checks. Logging errors are separate diagnostics. Documentation correcting the historical PDFs includes the 10-file board, Anteater restrictions, timer skip policy, AI-only repetition and four promotion types.
