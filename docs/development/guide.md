# Development and release

## Toolchain and graph

Use C11, C++17, CMake >= 3.21, Ninja, GCC/Clang, Python 3 and Qt >= 6.4.2.
Qt modules are Core, Gui, Qml, Quick, QuickControls2, Svg, Test and QuickTest.
[README](../../README.md) lists Ubuntu 24.04 and MSYS2 UCRT64 package commands.
Windows packaging uses official windeployqt, objdump and pacman ownership records.
The active MSYS2 mount is queried with cygpath; installation paths may contain
spaces or use the CI runner's temporary directory.
Dependency versions and deployed hashes are recorded in each candidate's
DEPENDENCIES.json. Dependency upgrades require their own validation record.
The source API baseline is Qt 6.4.2; a newer local Windows package is not a
requirement to use newer APIs.

CMake owns all libraries, desktop, tests and benchmark targets. GNU Make forwards
commands to [build.py](../../tools/dev/build.py), with no second dependency graph.
Core-only commands set AC_BUILD_DESKTOP=OFF and do not discover Qt.

```sh
make CONFIG=release gui test test-platform test-gui check
make headless test              # no Qt required
make test-rules test-session test-ai
make CONFIG=sanitize test       # Linux ASan + UBSan
QT_QPA_PLATFORM=xcb ASAN_OPTIONS=detect_leaks=0 xvfb-run -a make CONFIG=sanitize test-gui
make CONFIG=release benchmark
```

Direct CMake usage:

```sh
cmake -S . -B build/local -G Ninja -DCMAKE_BUILD_TYPE=Release -DAC_BUILD_DESKTOP=ON
cmake --build build/local --parallel 4
ctest --test-dir build/local --output-on-failure
```

Outputs live under build/<platform>-x64/<configuration>; override BUILD for an
isolated tree. Debug, Release, Sanitize and Windows console builds stay separate.
With the current Windows MSYS2 Qt tools, use an ASCII BUILD path if the source
directory contains non-ASCII characters, for example BUILD=C:/ac-build/release.
The source and deployed runtime can remain in the Unicode directory.
Use CC/CXX or CMake cache variables to select compilers. C sources are always
compiled as C; a C++ linkage test exercises the guarded public headers.
Assertions remain enabled in Release tests. Each failing CTest result stops the
wrapper. Core sanitizer runs retain leak detection; desktop runs can disable it
for system Qt caches. Tests verify behavior rather than physical display quality.

## Desktop and ownership

The only desktop implementation is apps/qt/. app/ owns Session commands and
navigation, models/ owns copied board/history projections, async/ owns snapshot
search work, runtime/ owns paths/clocks/logs, and qml/ owns pages/components.
A worker never reads a live Session or a QML object. Cooperative cancellation is
atomic; completion validates page, generation, revision and close state. Never
release worker data before thread exit or force-terminate a search.

Qt resources embed the existing SVG aliases. QML pages remain alive during
ordinary moves/ticks, preserving keyboard focus and scroll position. Use injected
clocks and diagnostic callbacks for deterministic adapter tests.

Windows desktop builds use the GUI subsystem. Tests and benchmarks are console
executables. make WINDOWS_CONSOLE=1 gui produces a separate console desktop.
--version writes captured stdout; --smoke-test verifies all resources, starts and
finishes a Session, checks executable-relative logs, and on Windows rejects loaded
GTK/Cairo/GdkPixbuf modules. A log/resource failure returns 1. Linux symlinks resolve
to the actual executable. Accepted Session commands survive log failure.

## Working conventions

Public headers belong in include/anteater with ac_/Ac/AC_ names and C++ linkage
guards. Follow .clang-format. Explain ownership, contracts and non-obvious reasons
in comments. Rules owns legality, Session owns transactions/history/clocks, AI
owns search contexts, and desktop code owns presentation. New C files and
core test_*.c files in existing module directories are discovered by CMake.
Cross-file AI evaluation helpers use narrow private headers; heuristic attack
estimates do not replace Rules legality. AI extraction must pass the immutable
[reference fixture](../../tests/fixtures/ai-evaluation-baseline.txt).

New special moves need generation, legality, apply/unmake, hash, resolver and
presentation coverage. Update observable contracts in the manual/spec. Preserve
COPYRIGHT, team credit, original PDFs and historical regression fixtures.

## Packages and verification

```sh
make CONFIG=release package-source package
python3 tools/packaging/verify.py
```

On Linux run the verifier with QT_QPA_PLATFORM=xcb under Xvfb if no display is
available; this also prevents a WSLg Wayland session from bypassing Xvfb. tar aliases
package-source; tar-user aliases package. Archives under dist/ contain source
or the host runtime. Source rebuilds require no Git checkout and retain historical
PDFs, tests, documentation and tools. Runtime packages retain COPYRIGHT, the user
manual and historical user PDF. Windows uses windeployqt then recursively verifies
non-system DLL imports and hashes, bundles package licenses, and rejects the retired
GTK/Cairo/GdkPixbuf chain. Some MSYS2 Qt font dependencies use GLib indirectly;
the manifest records those rather than claiming no GLib exists. Linux uses system
Qt packages. Its manifest combines the Qt QML import scanner, QML payloads,
WorkerScript, available GUI plugin families and their ELF dependency closures,
with package ownership, versions and file hashes. Available styles/platform/image
backends are recorded as capabilities, not as evidence that all were loaded.

Input byte hashes invalidate stale archives. Linux also fingerprints its dependency
manifest, so QML/plugin/library changes invalidate a cached runtime package even
when the application executable is unchanged. SHA256SUMS covers all archives.
The verifier extracts to a Unicode/spaces path, rebuilds core and desktop without
Git, checks source-package invalidation and PDF bytes, then checks --version,
concurrent logs, repeated sessions, different cwd, symlink launch on Linux and
blocked log directories. Windows additionally tests GUI/console PE subsystems,
manifest hashes, and execution with a PATH containing only the runtime/System32.
This is dependency isolation on the local host, not a separately provisioned VM.

## CI and publication

The workflow defines Ubuntu 24.04/Xvfb/sanitizer and Windows UCRT64 checks and
candidate archives. Local successful runs do not prove the updated remote CI has
run. Use the same commit for both release jobs; inspect hashes and validation,
then publish the full set through the existing draft-release workflow. No release
is automatically published by local packaging. No macOS acceptance is claimed.
See [validation](validation.md) for exact evidence and outstanding human checks.
