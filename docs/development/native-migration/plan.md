# C++20 / MSVC / official Qt migration

Accepted scope: the user instruction of 2026-10-05. This local execution record
does not replace current repository documentation before the environment gate.

## Baseline and gate

- Start exclusively from clean commit `32da9759f3dfe1555de57b26c3d3b37ec4636f2a`.
- Preserve Git history, COPYRIGHT, team attribution, historical PDF bytes, original
  rules fixtures, and the frozen AI score/SEE/move/depth/node reference data.
- Windows: MSVC v143 x64, official Qt 6.11.2 MSVC 2022 x64 binaries, native
  CMake/Ninja/Python. Install tools on C:; keep project and outputs on D:.
- Linux: Ubuntu 24.04, system GCC, system Qt 6.4.2.
- Compile and run a C++20/Qt probe with the selected compiler and Qt before
  migrating source. ABI compatibility alone is insufficient.
- Installation or kit failure keeps the tracked checkout at the baseline. No new
  UCRT64 fallback, compatibility stack, transition release, or automatic Release.

## Reviewable commit sequence after the gate

1. Build graph and environment: CMake >= 3.25, CXX-only C++20; Windows/Linux
   Debug, Release, core-only, and Linux sanitizer presets. A process-local
   PowerShell environment script selects and verifies v143, official Qt, and
   native tools. Ignore local `CMakeUserPresets.json`. Core-only builds avoid Qt;
   disabling tests and developer tools also avoids Python. Retire Make forwarding.
2. Rule and evaluation values: `ac` namespace, `.hpp`, scoped enums, `std::array`,
   semantic equality, explicit `Status` and `Result<T>` using `std::variant<T, Error>`.
   Preserve integer arithmetic, traversal, stable ordering, evaluation weights,
   SEE, pruning, move ordering, and deterministic search. Delete C API wrappers.
3. Session ownership: noncopyable, movable RAII with injectable clock and
   `std::pmr::memory_resource`; transactional allocation errors. Separate cheap
   `SessionState` from owning `SessionSnapshot`; retain revision/gameId, capacity,
   overflow, all mode policies, timeout, undo, Tournament no-refund, repetition,
   and valid difficulty values 0/1/2/3/5. Move budgets/configuration policy out of AI.
   Remove log writes from Session; desktop records committed state diagnostically.
4. Search ownership: noncopyable/movable RAII context and complete owned requests;
   synchronous spans never outlive task data. AI receives explicit limits and
   positions. Preserve input on cancellation and test every allocation failure.
5. Desktop cutover: sole Session-owning application controller; distinct settings
   draft, input, board, history, clock, and status models with narrow signals.
   Registered QML types and named enums replace context properties and bare
   numeric conventions. Preserve focus/scroll and avoid history/board refresh on
   100 ms clock ticks. A single QThread task uses stop_source/stop_token and checks
   gameId/revision/generation/closing before acceptance. Stop commands, cancel,
   join, then release ownership on close; preserve failed-search suppression.
6. Distribution: qt_add_qml_module and official Qt deployment CMake APIs;
   CMake/CTest/CPack own configure/build/test/install/archive. Windows dynamic Qt
   ZIPs use only permitted Release REDIST runtime sources, record provenance,
   licenses, versions and hashes, validate EXE/plugin/DLL closures, and reject
   missing dependencies, GTK/Cairo, or MinGW runtimes. Linux retains system Qt
   dependency/QML/plugin records. Remove project pacman/cygpath/UCRT64 branches,
   custom archive caches and duplicate builds; keep machine MSYS2 installed.
7. Cleanup and authority: retain SVG bytes with unified names, remove WhiteAntsvg
   special cases, legacy probes and AC_LEGACY_PROBE dual-engine tests, migrate
   current comparators without changing expected data, archive old migration
   documents, update English architecture/manual/development/CI documentation,
   and validate current path references. Source archives exclude generated/local
   files and rebuild without Git.

## Required verification ledger

- Original rule baseline, all special moves, semantic apply/unmake equality,
  hashes, capacity, transaction failures, every allocation failure cleanup, and
  snapshot validity after Session mutation/destruction.
- All three modes: undo, timeout, Tournament no-refund, automatic repetition,
  difficulty validation, budgets, and unchanged AI strategy.
- Frozen AI scores, SEE, selected moves, depth, and nodes match MSVC and GCC;
  expected data is never updated to conceal a regression.
- Search overlapping move/restart/undo/timeout/navigation/close and late results.
- Mouse/coordinate input, promotion confirmation/cancel, focus, history scrolling,
  fullscreen restoration, 100%/150%/200% scaling, and software rendering.
- Spaces/Unicode paths, changed cwd, concurrent instances, unwritable logs, path
  discovery failures, Linux symlinks, monotonic clocks, actual EXE placement,
  per-game isolation, and QSaveFile atomic replacement.
- Both platforms: source rebuild without Git, candidates, license/dependency
  records, PDF checksums, CI, and Linux ASan/UBSan. Windows also starts on a clean
  environment without Qt, MSYS2, Visual Studio, or preinstalled VC developer runtime.
- Record local evidence, remote CI, and manual desktop acceptance separately.
  Source migration and release acceptance are separate statuses; pending external
  checks stay pending. Failed migration commits roll back to the baseline without
  leaving a second supported core or build chain. No automatic publication.

## Current status

Baseline confirmed clean. Environment gate blocked: v143 installation needs
elevation; the UAC retry failed with cancellation. No migration commit started.
See `REPORT.md` and the adjacent logs for actual evidence.
