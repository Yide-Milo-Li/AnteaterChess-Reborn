# AnteaterChess Reborn

A C11 / GTK 3 desktop chess variant on an **8 × 10 board**, with Ants and chain-capturing Anteaters. Reborn separates rules, sessions, search, and desktop presentation while retaining the original game's layout and gameplay.

Choose Human vs Human, Human vs AI, or AI vs AI. The desktop includes legal-move highlighting, typed coordinates, promotion selection, hints, undo, configurable turn timers, fullscreen, and game logs. A turn timeout skips the turn; Tournament search has its own time pool. Automatic threefold repetition applies only to AI vs AI.

## Run

Download the platform archive from [Releases](https://github.com/Yide-Milo-Li/AnteaterChess-Reborn/releases). Windows x64: extract the entire ZIP and run `anteater-chess.exe`; keep its DLLs and support directories together. Ubuntu 24.04 x64: install `libgtk-3-0t64` and `librsvg2-common`, extract the archive, and run `./anteater-chess`. The runtime archives include installation instructions and the current manual.

## Build

Ubuntu 24.04:

```sh
sudo apt-get update
sudo apt-get install build-essential pkg-config libgtk-3-dev librsvg2-common python3
make -j4
make test
make run
```

Windows: install MSYS2 and open its **UCRT64** shell:

```sh
pacman -S --needed make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-gtk3 mingw-w64-ucrt-x86_64-librsvg mingw-w64-ucrt-x86_64-python
make -j4
make test
make run
```

`make headless test` needs only C11, GNU Make, and the usual shell tools; it does not query GTK. Use `CONFIG=release` for optimized builds. See the [developer guide](docs/DEVELOPMENT.md) for all targets and packaging.

## Project map

| Directory | Responsibility |
| --- | --- |
| `include/anteater/` | Public C interfaces and values |
| `src/rules/` | Move generation, legality, reversible positions, hashes |
| `src/session/` | History, commands, clocks, result, diagnostics |
| `src/ai/` | Independent search contexts, evaluation, ordering, cache, budgets |
| `src/platform/` | GLib clock and writable-user-directory logs |
| `apps/gtk/` | Pages, input, rendering, GTask workers |
| `assets/` | SVG resources embedded at build time |
| `tests/` | Rules, session, AI, desktop tests, fuzzing and baseline fixtures |
| `mk/`, `tools/` | Build, checks, packaging and validation |
| `docs/` | Current documentation; original PDFs in `legacy/` |

## Documentation

- [User manual](docs/Chess_UserManual.md): rules, controls, setup, troubleshooting.
- [Software specification](docs/Chess_SoftwareSpec.md): ownership, contracts, lifecycle, module/state diagrams.
- [Development](docs/DEVELOPMENT.md): tools, testing, conventions, packaging and release.
- [Migration](docs/MIGRATION.md): baseline, old paths and interface replacements.
- [Test migration inventory](docs/TEST-MIGRATION.md): disposition of original scenarios.
- [Release notes](docs/RELEASE-NOTES.md) and [validation record](docs/VALIDATION.md).
- [Agent instructions](AGENTS.md).
- Historical course PDFs: [user manual](docs/legacy/Chess_UserManual.pdf), [software specification](docs/legacy/Chess_SoftwareSpec.pdf). These are preserved unchanged and describe an older implementation. Current Markdown and executable tests take precedence.

## Authors and rights

Originally developed for UC Irvine EECS 22L by **Team 22: DeepAnteater**: Yao Li, Benjamin Feng, Yide Li, Yurang Li, Yasith Diunugala, and Max Zhang. The complete 215-commit baseline history is retained.

The original [COPYRIGHT](COPYRIGHT) terms remain unchanged. This is not an open-source license grant: course staff have the stated course-use permission; other rights remain reserved by the authors. Third-party dependencies retain their own licenses, included with the Windows distribution.
