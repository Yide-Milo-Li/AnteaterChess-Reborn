# AnteaterChess Reborn

A C11 / GTK 3 desktop chess variant on an **8 × 10 board**, featuring Ants and chain-capturing Anteaters. Reborn modernizes the original game with a decoupled architecture, high-definition visual design, and real-time HiDPI piece scaling while preserving full rules integrity and gameplay authenticity.

---

## Highlights & Features

- **Modern Obsidian Slate UI**: Clean, minimalist dark theme (`#0e1017` / `#161922`) with refined ivory/walnut board squares, warm gold glow accents, and monospace coordinate grids.
- **HiDPI Dynamic Piece Scaling**: Real-time adaptive SVG rasterization scales piece graphics dynamically ($32\text{px} \sim 160\text{px}$) with jitter filtering as the window resizes or toggles fullscreen.
- **Pure C11 Core Engine**: Complete decoupling of core rules, board representation, and AI search from GTK/GLib. The core library compiles independently without desktop dependencies.
- **Interactive Architecture Diagram**: Explorable standalone HTML architecture map with guided views and component boundaries in [docs/architecture.html](docs/architecture.html).
- **Multiple Game Modes**: Human vs Human, Human vs AI, and AI vs AI.
- **Desktop Controls & Ergonomics**: Click-to-move and typed coordinate input, valid move and hint highlighting, multi-level undo, promotion pickers, turn clocks, Tournament time pools, and game diagnostic logs.
- **Asynchronous AI Worker**: Background `GTask` thread isolation prevents UI freezing during deep search, with cooperative cancellation and session revision validation.

---

## Interactive Architecture

Explore the full system architecture, module boundaries, and data paths in the [Interactive Architecture Diagram](docs/architecture.html).

| Layer / Subsystem | Path | Responsibility |
| --- | --- | --- |
| **Desktop Shell** | `apps/gtk/` | GTK 3 windows, screens, event handling, dynamic rendering, worker threads |
| **Session Engine** | `src/session/` | Transactional state management, revision tracking, undo history, clock ticking |
| **Rules Engine** | `src/rules/` | Move generation, legality verification, 80-square board, Zobrist hashing |
| **AI Search** | `src/ai/` | Iterative deepening, alpha-beta pruning, transposition tables, budget management |
| **Platform Adapter** | `src/platform/` | Monotonic clock injection and user-writable diagnostic loggers |
| **Assets** | `assets/` | 18 vector SVG piece illustrations compiled into GResource |

---

## Quick Start

### Download Prebuilt Binaries

Platform archives are available under [Releases](https://github.com/Yide-Milo-Li/AnteaterChess-Reborn/releases).

- **Windows x64**: Extract the ZIP package and launch `anteater-chess.exe` (keep accompanying DLLs in place).
- **Ubuntu 24.04 x64**: Install runtime libraries (`sudo apt install libgtk-3-0t64 librsvg2-common`), extract, and run `./anteater-chess`.

---

## Build from Source

### Ubuntu 24.04

```sh
sudo apt-get update
sudo apt-get install build-essential pkg-config libgtk-3-dev librsvg2-common python3
make -j4
make test
make test-gui
make run
```

### Windows (MSYS2 UCRT64)

Install MSYS2 and run inside the **UCRT64** environment:

```sh
pacman -S --needed make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-gtk3 mingw-w64-ucrt-x86_64-librsvg mingw-w64-ucrt-x86_64-python
make -j4
make test
make test-gui
make run
```

### Headless & Release Builds

- Build and test core logic without GTK: `make headless test`
- Release optimized binaries: `make CONFIG=release gui test test-gui`
- Source & binary distribution packaging: `make CONFIG=release package-source package`

---

## Documentation

- [User manual](docs/Chess_UserManual.md): Gameplay rules, controls, setup options, troubleshooting.
- [Software specification](docs/Chess_SoftwareSpec.md): Ownership invariants, API contracts, lifecycle state machine.
- [Interactive Architecture Diagram](docs/architecture.html): Visual architecture layout with guided inspection views.
- [Development guide](docs/DEVELOPMENT.md): Build targets, test conventions, packaging, and release processes.
- [Migration guide](docs/MIGRATION.md): Baseline history, deprecated APIs, and interface mapping.
- [Test migration inventory](docs/TEST-MIGRATION.md): Mapping and status of original test scenarios.
- [Release notes](docs/RELEASE-NOTES.md) & [Validation record](docs/VALIDATION.md).
- [Agent instructions](AGENTS.md): Architectural invariants and pair-programming guidelines.
- Historical course PDFs: [User manual](docs/legacy/Chess_UserManual.pdf), [Software specification](docs/legacy/Chess_SoftwareSpec.pdf).

---

## Authors & Rights

Originally developed for UC Irvine EECS 22L by **Team 22: DeepAnteater**: Yao Li, Benjamin Feng, Yide Li, Yurang Li, Yasith Diunugala, and Max Zhang. The complete 215-commit baseline history is retained.

The original [COPYRIGHT](COPYRIGHT) terms remain unchanged. All rights remain reserved by the authors. Third-party dependencies retain their respective open-source licenses, bundled with binary distributions.
