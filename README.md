# AnteaterChess Reborn

A C++20 / Qt 6 + QML desktop chess variant on an **8 × 10 board**, with Ants and
chain-capturing Anteaters. The maintained implementation has a Qt-independent
core and preserves the original variant's rules and frozen AI regression results.

## Current version and status

Reviewed on 2026-10-05 against source commit
`cf2f30b7db09edac42cade7e7b61ad0db483d2a1`, now integrated into `main`. [VERSION](VERSION) remains `2.0.1`; identify native
packages by their `SOURCE_REVISION`, rather than the version number alone.

- C++20 source migration, Windows/Linux packages and no-Git source rebuilds are complete.
- [Windows and Linux CI](https://github.com/Yide-Milo-Li/AnteaterChess-Reborn/actions/runs/37358903929) passed for that source commit.
- Clean-Windows and manual desktop acceptance are complete per the user's 2026-10-05 confirmation. Individual acceptance receipts were not supplied; see the [validation ledger](docs/development/native-migration/validation.md).
- `main` includes the native migration. The native implementation has not been published as a GitHub Release.

The [published v2.0.1 release](https://github.com/Yide-Milo-Li/AnteaterChess-Reborn/releases/tag/v2.0.1)
is the earlier C11/GTK implementation at `6d92663`. Its archives do not contain
the current C++20/Qt implementation.

## Features

- Human vs Human, Human vs AI and AI vs AI.
- Ant movement, Anteater capture chains, castling, en passant and four promotion choices.
- Dark Qt Quick interface, SVG pieces rasterized for board size and display pixel ratio, and F11/Escape fullscreen controls.
- Click and coordinate input, legal-move and hint highlights, undo, turn timers and Tournament time budgets.
- Owning AI requests run on a QThread with cooperative cancellation; stale results are checked before application.
- Per-game diagnostic logs in `logs/` beside the actual executable.

The [user manual](docs/user/manual.md) defines the variant's rules and timing/undo
policies. Experimental difficulty has been removed. There is no networking or
saved-game import/export.

## Run a native package

Use a native archive delivered for the source revision above, or build and package
this checkout using the [development guide](docs/development/guide.md).

- **Windows x64:** extract the complete ZIP to a writable directory and launch `anteater-chess.exe`. Keep DLLs, `plugins/`, `qml/` and `qt.conf` with it. The game opens without a console; no development tools are required.
- **Ubuntu 24.04 x64:** extract the TGZ, install the system Qt dependencies listed in the package's `INSTALL.md`, then run `./anteater-chess` in a graphical session. The [Linux installation template](tools/packaging/templates/INSTALL-linux.md) lists those packages.

Each native archive includes `SOURCE_REVISION`, `FILES.sha256`, dependency/license
records and `INSTALL.md`. A different working directory or a Linux symlink does
not change the log location. macOS is outside the supported distribution targets.

## Build from source

### Windows: native PowerShell

Install Visual Studio's v143 **14.44 x64** component, a Windows SDK, native
CMake >= 3.25, Ninja and Python 3, and the official **Qt 6.11.2 MSVC 2022 x64**
kit with Shader Tools. The environment script defaults to
`C:\Qt\6.11.2\msvc2022_64`; initialize each new PowerShell process:

```powershell
. ./tools/native/Enter-NativeEnvironment.ps1 -RequireQt -RequirePython
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
./build/windows-release/bin/anteater-chess.exe
```

The Release desktop preset enables distribution receipts and therefore requires
Python. MinGW/MSYS2 is not a supported Windows build toolchain.

### Ubuntu 24.04

Use GCC, CMake >= 3.25, Ninja, Python 3 and system Qt 6.4.2:

```sh
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build python3 qt6-base-dev qt6-declarative-dev qt6-svg-dev qt6-shadertools-dev qt6-shader-baker qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts qml6-module-qtquick-window qml6-module-qtquick-templates qml6-module-qtqml-workerscript qml6-module-qttest
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release
./build/linux-release/bin/anteater-chess
```

### Core and sanitizer builds

`windows-debug-core`, `windows-release-core`, `linux-debug-core` and
`linux-release-core` build and test without Qt or Python discovery.
`linux-sanitizer` runs core tests with ASan/UBSan. Production core-only builds
disable desktop, testing, development tools and distribution; see the
[development guide](docs/development/guide.md).

## Architecture and documentation

| Module | Path | Responsibility |
| --- | --- | --- |
| Desktop | `apps/qt/{app,models,qml,async}/` | ApplicationController, typed models, pages and background jobs |
| Session | `src/session/` | Sole live game owner, transactions, history, revisions and clocks |
| Rules | `src/rules/` | Position, legal moves, reversible execution and endgame rules |
| Policy | `src/policy/` | Configuration validation, depth selection and Tournament budgets |
| AI | `src/ai/` | Owning search contexts/requests, evaluation and search |
| Runtime | `apps/qt/runtime/` | Executable paths, injected monotonic clock and atomic diagnostic logs |

Start at the [documentation index](docs/README.md). The
[software specification](docs/architecture/software-specification.md) defines
current contracts; the [interactive architecture](docs/architecture/architecture.html)
shows their relationships. Build, test and packaging instructions are in the
[development guide](docs/development/guide.md), with measured results in the
[native validation ledger](docs/development/native-migration/validation.md).

[Historical documents](docs/legacy/README.md) are archived and receive no ongoing
behavior updates. The current manual and specification are authoritative.

## Authors and rights

Originally developed for UC Irvine EECS 22L by **Team 22: DeepAnteater**:
Yao Li, Benjamin Feng, Yide Li, Yurang Li, Yasith Diunugala and Max Zhang.
The original 215-commit course history is retained.

[COPYRIGHT](COPYRIGHT) and team attribution remain unchanged. All rights remain
reserved by the authors. Third-party dependencies retain their respective
licenses; native packages include dependency and license inventories.
