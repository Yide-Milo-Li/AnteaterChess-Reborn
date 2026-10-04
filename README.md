# AnteaterChess Reborn

A C11 / Qt 6 + QML desktop chess variant on an **8 × 10 board**, featuring Ants and chain-capturing Anteaters. Reborn modernizes the original game with a decoupled architecture, high-definition visual design, and real-time HiDPI piece scaling while preserving full rules integrity and gameplay authenticity.

---

## Highlights & Features

- **Modern Obsidian Slate UI**: Clean, minimalist dark theme (`#0e1017` / `#161922`) with refined ivory/walnut board squares, warm gold glow accents, and monospace coordinate grids.
- **HiDPI Dynamic Piece Scaling**: Real-time adaptive SVG rasterization scales piece graphics dynamically ($32\text{px} \sim 160\text{px}$) through Qt Quick as the window resizes or toggles fullscreen.
- **Pure C11 Core Engine**: Complete decoupling of core rules, board representation, and AI search from Qt/GTK/GLib. The core library compiles independently without desktop dependencies.
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

```sh
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build python3 qt6-base-dev qt6-declarative-dev qt6-svg-dev qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts qml6-module-qtquick-window qml6-module-qtquick-templates qml6-module-qtqml-workerscript qml6-module-qttest
make -j4
make test
make test-gui
make run
```

### Windows (MSYS2 UCRT64)

Install MSYS2 and run inside the **UCRT64** environment:

```sh
pacman -S --needed make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-declarative mingw-w64-ucrt-x86_64-qt6-svg mingw-w64-ucrt-x86_64-python
make -j4
make test
make test-gui
make run
```

### Headless & Release Builds

- Build and test core logic without Qt: `make headless test`
- Release optimized binaries: `make CONFIG=release gui test test-gui`
- Source & binary distribution packaging: `make CONFIG=release package-source package`

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
