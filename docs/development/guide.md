# Development and release

## Toolchain and builds

The maintained graph uses C++20, CMake >= 3.25 and Ninja. Windows selects native
MSVC v143 14.44 x64, Windows SDK, official Qt 6.11.2 MSVC 2022 x64 with Shader
Tools, and native Python. Linux uses Ubuntu 24.04, GCC and system Qt 6.4.2.
Tools stay on C: and the checkout, build trees and evidence stay on D:.

In every new PowerShell process, dot-source
[Enter-NativeEnvironment.ps1](../../tools/native/Enter-NativeEnvironment.ps1).
It imports vcvarsall x64 14.44 into that process, selects native tools and Qt,
and validates actual compiler resolution. It never changes the global PATH.
The `-RequireQt` switch also checks Debug/Release Qt Core and Shader Tools.
`-RequirePython` checks the optional native Python prerequisite for tests/tools;
core consumers with both disabled do not need Python.
`-QtRoot` overrides the kit path; `-ReceiptPath` records the selected environment.

```powershell
. ./tools/native/Enter-NativeEnvironment.ps1 -RequireQt
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
```

Linux uses the equivalent `linux-debug`/`linux-release` presets. The eight
Windows/Linux Debug/Release desktop/core presets each use an isolated build tree.
The `linux-sanitizer` preset enables address and undefined-behavior sanitizers.
For GUI checks with a physical X11 backend use Xvfb and `QT_QPA_PLATFORM=xcb`.
Offscreen software rendering provides deterministic automated QML checks;
it does not establish physical desktop or DPI acceptance.

Core-only presets disable desktop discovery. Production core consumers can omit
both Python and Qt entirely:

```sh
cmake -S . -B build/core-only -G Ninja -DCMAKE_BUILD_TYPE=Release -DAC_BUILD_DESKTOP=OFF -DBUILD_TESTING=OFF -DAC_BUILD_DEV_TOOLS=OFF
cmake --build build/core-only
```

CMake owns libraries, executables and tests. GNU Make forwarding is retired.
`AC_BUILD_DEV_TOOLS=ON` adds the benchmark and `check` target. Assertions remain
enabled in tests in Release configurations. New maintained implementation and
test sources use `.cpp`; the compiler accepts no C language target.

## Contracts and migration sequencing

Rules owns legality and integer board/search semantics, Session owns transactions,
history, clocks and mode policies, AI owns search workspaces, and Qt owns logs,
paths, navigation and presentation. A worker must own everything it reads until
its thread exits. Accepted game commands survive diagnostic log failures.
Preserve timeout/undo, Tournament budgets and AI-only repetition policy.

The approved [migration plan](native-migration/plan.md) and
[validation ledger](native-migration/validation.md) govern the current staged
change. The build stage compiles the existing algorithms as C++20; subsequent
stages replace the C-shaped interfaces and ownership. Do not treat compilation
alone as Session or search lifetime acceptance.

Frozen AI output includes scores, SEE, legal moves, chosen moves, completed depth
and node counts. Compare both compilers with the unchanged
[fixture](../../tests/fixtures/ai-evaluation-baseline.txt). Never regenerate it
to accommodate a migration difference. Historical probe data, PDF/SVG bytes,
COPYRIGHT and team credit remain protected by
[the baseline inventory](native-migration/protected-files.json).

## Evidence and distribution

Each stage must pass its required checks before the next stage is committed.
Record tested commit, exact command, environment, real exit code and attachment
SHA-256. Preserve raw receipts outside `build/`; cleanup may delete build trees.
Local runs, remote CI and human acceptance have separate ledger entries.

Distribution is being migrated to CMake install/CPack and official Qt deployment.
Earlier custom packaging scripts and their reports remain historical during this
stage and do not establish native candidates. The final source and two platform
candidates must share the tested commit and include dependency/license manifests,
checksums and validation attachments. Clean Windows and human desktop checks
remain pending when no acceptance environment is supplied. Do not publish a
Release automatically.
