# C++20 native migration map

The original 215-commit course history remains in this independent repository.
The current migration starts exclusively at Reborn commit
`32da9759f3dfe1555de57b26c3d3b37ec4636f2a` on
`codex/cpp20-native-migration`. The original repository receives no changes.

| Previous interface | Maintained replacement |
| --- | --- |
| C API and .h/.c core | ac namespace, scoped enums, .hpp/.cpp and C++20 |
| Opaque borrowed Session snapshots | noncopyable movable Session; allocation-free SessionState and owning SessionSnapshot |
| AI budget/configuration helpers | independent ac_policy library |
| Search context + borrowed options | RAII SearchContext and complete owning SearchRequest |
| SessionAdapter/context property | sole ApplicationController and registered typed QML models |
| Unified stateChanged | separate settings, input, clock, status, board and history notifications |
| Atomic flag worker | one QThread task with stop_source/stop_token and complete identity checks |
| Hand-written QML manifest | qt_add_qml_module registration/resources/import plugin |
| Make wrappers and custom archives | CMake presets, CTest, install and CPack |
| Windows compiler package manager | native v143 14.44 and official Qt 6.11.2 MSVC 2022 x64 |
| White Ant resource exception | WhiteAnt.svg for both physical file and resource alias |

There is no supported C ABI or retained wrapper. Existing SVG/PDF/COPYRIGHT and
frozen expected data remain byte-identical. The required resource manifest alias
change is documented in [structural changes](native-migration/structural-changes.json).
The original [probe](../legacy/probes/legacy_probe.c) and frozen dual-engine
fixture remain uncompiled historical evidence. The maintained comparisons are
single C++ implementations and never regenerate expected values.

Rules, integer evaluation/search order, Tournament allocation/no-refund, timeout
skip, undo and AI-only repetition remain as described in [the manual](../user/manual.md).
Logs are desktop diagnostics after an accepted command. Workers own inputs and
cannot access Session; late results require matching page, gameId, revision,
generation and closing state.

Previous C11/GTK and C11/C++17 Qt migration reports are under
[the historical archive](../legacy/migrations/). They do not establish acceptance
for this native migration. The [current ledger](native-migration/validation.md)
separates local checks, remote CI and external acceptance.
