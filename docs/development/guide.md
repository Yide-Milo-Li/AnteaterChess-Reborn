# Development and candidate verification

Use one C++20 CMake >= 3.25/Ninja graph, CTest and CPack. Windows requires native
v143 14.44 x64 and the official Qt 6.11.2 MSVC 2022 x64 kit with Shader Tools.
Ubuntu 24.04 uses GCC and system Qt 6.4.2. Core and core tests need neither Qt
nor Python. Optional developer checks and dependency/license receipts use Python.

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
cmake -S . -B build/core-only -G Ninja -DCMAKE_BUILD_TYPE=Release -DAC_BUILD_DESKTOP=OFF -DBUILD_TESTING=OFF -DAC_BUILD_DEV_TOOLS=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE
cmake --build build/core-only
```

AC_BUILD_DEV_TOOLS=ON adds ac_benchmark and the check target. Tests preserve
assertions in Release. The single C++ AI comparator reads the frozen fixture
directly; never regenerate expected scores, SEE, move fingerprints, depth or nodes.

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
source/runtime archives to their configured commit and file inventory.
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
Windows startup clears developer Qt paths; this is local relocation evidence,
not an independent clean Windows host.

## CI and evidence

CI uses native Windows v143 and official Qt archives, and Ubuntu GCC/system Qt,
Xvfb plus ASan/UBSan. Its artifacts include the tested commit, commands, real exit
codes and attachment hashes. No publishing workflow runs automatically.
The [native ledger](native-migration/validation.md) separates source migration,
local candidates, remote CI and clean Windows/human acceptance. Preserve raw
evidence outside build; cleanup only removes verified generated directories.

The [specification](../architecture/software-specification.md) defines owners,
resource lifetimes and transaction boundaries. Rules, AI integer ordering,
Tournament allocation/no-refund, undo/timeout and AI-only repetition are frozen.
QML uses typed required models and enums, and ordinary ticks update only clocks.
