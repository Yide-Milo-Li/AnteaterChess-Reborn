# Development and packaging

Reviewed on 2026-10-05 against source commit `cf2f30b`. The native migration's
local packages and remote CI passed; clean-Windows/manual acceptance is complete
per user confirmation. See the [native ledger](native-migration/validation.md)
for evidence and its limits. The native migration is integrated into main.
No native Release is published.
VERSION remains 2.0.1, also used by the older published C11/GTK implementation.

Use one C++20 CMake >= 3.25/Ninja graph, CTest and CPack. Windows requires native
v143 14.44 x64 and the official Qt 6.11.2 MSVC 2022 x64 kit with Shader Tools.
Ubuntu 24.04 uses GCC and system Qt 6.4.2. Core and core tests need neither Qt
nor Python. Optional developer checks and dependency/license receipts use Python;
both Release desktop presets enable distribution and require it.

Initialize every new PowerShell process; the script imports the selected x64
toolset, checks real tool paths, selects the matching Release REDIST and filters
machine MSYS paths only in that process:

```powershell
. ./tools/native/Enter-NativeEnvironment.ps1 -RequireQt -RequirePython
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
```

Linux uses the equivalent linux-release commands. Eight isolated presets cover
Windows/Linux Debug/Release desktop/core; linux-sanitizer enables ASan/UBSan.
For native desktop checks use QT_QPA_PLATFORM=windows or Linux xcb under Xvfb.
CTest defaults to offscreen/software/Basic for automation; Windows offscreen
glyph rendering is not visual acceptance. tools/packaging/dpi_checks.py measures
native window DPI, then calibrates QT_SCALE_FACTOR to actual device pixel ratios
1, 1.5 and 2 with software rendering. Every captured window verifies that ratio;
the machine's display settings remain unchanged. See the
[Qt high-DPI testing documentation](https://doc.qt.io/qt-6.11/highdpi.html#testing).

Production core-only consumers also work with package discovery disabled:

```sh
cmake -S . -B build/core-only -G Ninja -DCMAKE_BUILD_TYPE=Release -DAC_BUILD_DESKTOP=OFF -DBUILD_TESTING=OFF -DAC_BUILD_DEV_TOOLS=OFF -DAC_BUILD_DISTRIBUTION=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE
cmake --build build/core-only
```

AC_BUILD_DEV_TOOLS=ON adds ac_benchmark and the check target. Tests preserve
assertions in Release. The single C++ AI comparator reads the frozen fixture
directly; never regenerate expected scores, SEE, move fingerprints, depth or nodes.

Run documentation/resource checks directly with native Python:

```sh
python tools/dev/check.py
git diff --check
```

To enable the benchmark and CMake check target, configure a development preset
with `-DAC_BUILD_DEV_TOOLS=ON`; then build `ac_benchmark` or `check` in that build
directory. For an isolated Windows console build:

```powershell
cmake --preset windows-debug -B build/windows-debug-console -DAC_WINDOWS_CONSOLE=ON
cmake --build build/windows-debug-console --target anteater-chess
./build/windows-debug-console/bin/anteater-chess.exe
```

## Install and CPack

Release desktop presets enable AC_BUILD_DISTRIBUTION and dependency receipts.
Debug redistributables are rejected. Installation uses Qt's official CMake
deployment API on Windows with compiler-runtime auto-copy disabled; only the
verified v143 Release REDIST is copied. SDK DirectX compiler files must match
the SDK's D3D x64 REDIST source, with its license recorded. Windows system DLLs
remain external. Every EXE, Qt plugin/QML DLL and runtime has an x64 import closure.

Qt 6.4.2's Linux deployment API cannot bundle runtime libraries and generates an
unquoted support include on space paths. The installed adapter quotes that path
and deploys only application QML imports through the public API. System Qt stays
external. The receipt independently requires QML modules, image/platform plugins,
ELF closures, Ubuntu package ownership/version and copyright/common-license texts.
A skip message alone never counts as successful dependency verification.

```sh
cmake --install build/linux-release --prefix dist/linux-stage
cpack --config build/linux-release/CPackConfig.cmake
cpack --config build/linux-release/CPackSourceConfig.cmake
```

Use build/windows-release for the equivalent native commands. The install graph
owns runtime contents; CPack owns ZIP/TGZ creation and SHA-256 sidecars. No custom
archive cache or Make wrapper remains. SOURCE_REVISION and FILES.sha256 bind
source/runtime archives to their configured commit and file inventory. A dirty
checkout is marked `-dirty` and is unsuitable as a formal clean-commit candidate.
DEPENDENCIES.json records actual dependencies, origins, licenses and payload hashes.
The source archive excludes Git and generated/local data and includes its revision.

Verify candidates in a fresh persistent directory (native environment remains
initialized for the optional rebuild):

```sh
python3 tools/packaging/verify_candidate.py --runtime dist/AnteaterChess-Reborn-2.0.1-linux-x64.tar.gz --source dist/AnteaterChess-Reborn-2.0.1-source.tar.gz --work ../migration-evidence/candidate-linux --rebuild
```

Windows uses python and the windows-x64.zip runtime. The verifier checks all file
hashes, protected data, commit identity, Unicode/space extraction, unrelated cwd,
concurrent logs, blocked logs, Linux symlinks and a source rebuild without Git.
Windows startup clears developer Qt paths. The verifier establishes local
relocation and rebuild evidence; independent clean-Windows and human acceptance
are recorded separately in the native ledger. Rebuilding documentation creates
new archive bytes and checksums even when the application code is unchanged;
previous delivery hashes continue to identify the original tested archives.

Unicode Windows source paths use a CMake-built adapter around the unmodified
official qmlimportscanner. Qt 6.11 decodes response files using the local ANSI
code page, while CMake writes UTF-8. The adapter expands those lines into a wide
command line and returns the actual child exit. This keeps import scanning and
deployment enabled without modifying the installed kit or system code page.

## CI and evidence

CI uses native Windows v143 and official Qt archives, and Ubuntu GCC/system Qt,
Xvfb plus ASan/UBSan. Its artifacts include the tested commit, commands, real exit
codes and attachment hashes. No publishing workflow runs automatically.
The [native ledger](native-migration/validation.md) separates source migration,
local packages, remote CI and clean Windows/human acceptance. Preserve raw
evidence outside build; cleanup only removes verified generated directories.

The [specification](../architecture/software-specification.md) defines owners,
resource lifetimes and transaction boundaries. Rules, AI integer ordering,
Tournament allocation/no-refund, undo/timeout and AI-only repetition are frozen.
QML uses typed required models and enums, and ordinary ticks update only clocks.
