# AnteaterChess Reborn

A C++20 / Qt 6 + QML desktop chess variant on an **8 × 10 board**, featuring Ants and chain-capturing Anteaters. Reborn modernizes the original game with a decoupled architecture, high-definition visual design, and real-time HiDPI piece scaling while preserving full rules integrity and gameplay authenticity.

---

## Highlights & Features

- **Modern Obsidian Slate UI**: Clean, minimalist dark theme (`#0e1017` / `#161922`) with refined ivory/walnut board squares, warm gold glow accents, and monospace coordinate grids.
- **HiDPI Dynamic Piece Scaling**: Real-time adaptive SVG rasterization scales piece graphics dynamically ($32\text{px} \sim 160\text{px}$) through Qt Quick as the window resizes or toggles fullscreen.
- **Independent C++20 Core Engine**: Complete decoupling of core rules, board representation, and AI search from Qt/GTK/GLib. The core library compiles independently without desktop dependencies.
- **Interactive Architecture Diagram**: Explorable standalone HTML architecture map with guided views and component boundaries in [docs/architecture/architecture.html](docs/architecture/architecture.html).
- **Multiple Game Modes**: Human vs Human, Human vs AI, and AI vs AI.
- **Desktop Controls & Ergonomics**: Click-to-move and typed coordinate input, valid move and hint highlighting, multi-level undo, promotion pickers, turn clocks, Tournament time pools, and game diagnostic logs.
- **Asynchronous AI Worker**: A snapshot-owning `QThread` worker keeps search off the UI thread, with cooperative cancellation and session revision validation.

---

## Interactive Architecture

Explore the full system architecture, module boundaries, and data paths in the [Interactive Architecture Diagram](docs/architecture/architecture.html).

| Layer / Subsystem | Path | Responsibility |
| --- | --- | --- |
| **Desktop Shell** | `apps/qt/{app,models,qml,async}/` | Application commands, pages, reusable presentation, worker threads |
| **Session Engine** | `src/session/` | Transactional state management, revision tracking, undo history, clock ticking |
| **Rules Engine** | `src/rules/` | Move generation, legality verification, 80-square board, Zobrist hashing |
| **AI Search** | `src/ai/` | Iterative deepening, alpha-beta pruning, transposition tables, budget management |
| **Platform Adapter** | `apps/qt/runtime/` | Executable paths, monotonic clocks and atomic diagnostic logs |
| **Assets** | `assets/{pieces,icons}/` | 14 chess pieces and 4 UI icons compiled into Qt resources |

---

## Quick Start

### Download Prebuilt Binaries

Previously published archives are available under [Releases](https://github.com/Yide-Milo-Li/AnteaterChess-Reborn/releases).

- **Windows x64**: Extract the ZIP package and launch `anteater-chess.exe` (keep accompanying DLLs in place).
- **Ubuntu 24.04 x64**: Install runtime libraries (`sudo apt install libqt6quick6 libqt6quickcontrols2-6 libqt6svg6 qml6-module-qtquick qml6-module-qtquick-window qml6-module-qtquick-controls qml6-module-qtquick-layouts qml6-module-qtquick-templates qml6-module-qtqml-workerscript`), extract, and run `./anteater-chess`.

The Qt migration produces local candidates under `dist/`; see [validation](docs/development/validation.md) for their acceptance status.

Extract to a writable directory. Windows game builds open without a console window.
On both platforms, game logs are stored in `logs/` beside the actual executable.

---

## Build from Source

### Ubuntu 24.04

Install GCC, CMake >= 3.25, Ninja and system Qt 6.4.2 development/QML modules.
Optional development checks and candidate dependency receipts use native Python 3.

```sh
sudo apt-get install build-essential cmake ninja-build python3 qt6-base-dev qt6-declarative-dev qt6-svg-dev qt6-shadertools-dev qt6-shader-baker qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts qml6-module-qtquick-window qml6-module-qtquick-templates qml6-module-qtqml-workerscript qml6-module-qttest
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release
./build/linux-release/bin/anteater-chess
```

### Windows (native PowerShell)

Install the v143 14.44 x64 component in Visual Studio, a Windows SDK, native
CMake/Ninja/Python, and the official Qt 6.11.2 MSVC 2022 x64 kit with Shader Tools
under `C:\Qt`. Initialize **each new PowerShell process** before configuring:

```powershell
. ./tools/native/Enter-NativeEnvironment.ps1 -RequireQt
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
./build/windows-release/bin/anteater-chess.exe
```

### Core and sanitizer builds

Use `linux-debug-core`, `linux-release-core`, `windows-debug-core` or
`windows-release-core` to compile/test without discovering Qt. Use
`linux-sanitizer` for ASan/UBSan. With `BUILD_TESTING=OFF` and
`AC_BUILD_DEV_TOOLS=OFF`, core-only builds do not require Python.

The [native migration ledger](docs/development/native-migration/validation.md)
records stage status. Existing release archives describe earlier implementations;
they are not candidates for the current migration. CMake install and CPack own
the current archives; each includes a source revision, dependency/license
inventory and checksums. External acceptance remains pending until the ledger
records its checks.

---

## Documentation

- [Documentation index](docs/README.md): Navigation by audience and purpose.
- [User manual](docs/user/manual.md): Gameplay rules, controls, setup options, troubleshooting.
- [Software specification](docs/architecture/software-specification.md): Ownership invariants, API contracts, lifecycle state machine.
- [Interactive Architecture Diagram](docs/architecture/architecture.html): Visual architecture layout with guided inspection views.
- [Development guide](docs/development/guide.md): Build targets, test conventions, packaging, and release processes.
- [Migration guide](docs/development/migration.md): Baseline history, deprecated APIs, and interface mapping.
- [Test migration inventory](docs/development/test-migration.md): Mapping and status of original test scenarios.
- [Release notes](docs/development/release-notes.md) & [Validation record](docs/development/validation.md).
- [Agent instructions](AGENTS.md): Architectural invariants and pair-programming guidelines.
- Historical course PDFs: [User manual](docs/legacy/Chess_UserManual.pdf), [Software specification](docs/legacy/Chess_SoftwareSpec.pdf).

---

## Authors & Rights

Originally developed for UC Irvine EECS 22L by **Team 22: DeepAnteater**: Yao Li, Benjamin Feng, Yide Li, Yurang Li, Yasith Diunugala, and Max Zhang. The complete 215-commit baseline history is retained.

The original [COPYRIGHT](COPYRIGHT) terms remain unchanged. All rights remain reserved by the authors. Third-party dependencies retain their respective open-source licenses, bundled with binary distributions.
