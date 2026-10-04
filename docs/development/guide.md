# Development and release

## Toolchain

Use C11, GNU Make, GCC, pkg-config, Python 3 and GTK 3 development packages. [README](../../README.md) lists Windows UCRT64 and Ubuntu 24.04 setup commands. Use the UCRT64 shell, not the MSYS or MINGW64 environment. Runtime GUI adapters also require GLib >= 2.72. `glib-compile-resources` is supplied with GLib. Windows packaging needs `objdump`, `pacman`, GdkPixbuf loaders and their licenses.

## Build and test

```sh
make -j4 CONFIG=release
make test                    # core, no GTK required
make test-rules
make test-session
make test-ai
make test-platform           # GLib adapter tests, no display required
make test-gui                # requires a desktop/display
make check                   # documentation/resources/layout checks
make CONFIG=sanitize test    # Linux: ASan + UBSan
xvfb-run -a make test-gui     # Linux headless desktop automation
```

Core-only: `make headless test GTK_CFLAGS= GTK_LIBS= GLIB_CFLAGS=`. Targets do not evaluate GTK variables unless needed. Outputs live in `build/<platform>-x64/<configuration>/`; Debug, Release, and Sanitize do not share objects. GCC dependency files track included headers; the resource target depends on its manifest and SVG files. Override `CC`, `AR`, `CPPFLAGS`, `CFLAGS`, `LDFLAGS`, `BUILD` as needed; retain required language/include options when overriding flags. `make clean` removes only the checked `build` and `dist` generated directories.

Tests use `assert`, including in Release, so do not define `NDEBUG`. GUI tests invoke widget handlers and the GTK loop; interactive visual quality still requires human review. Sanitizer GUI runs disable leak detection for process-global GTK caches, while core sanitizer tests retain it. Each test is a separate executable; a failing command stops the target.

## Repository layout

Windows game targets use the GUI subsystem in both Debug and Release. Tests and
benchmarks remain console executables. Use `make WINDOWS_CONSOLE=1 gui` for a
console-attached debug game; its default output is `build/windows-x64/debug-console/`.
The suffix also applies to Release console builds, keeping normal and console
objects/executables separate. If overriding `BUILD`, choose a separate directory
for each subsystem. `--version` supports captured stdout, and `--smoke-test`
returns a process status for automated verification.

`--smoke-test` checks all embedded pieces/icons and starts/finishes a minimal
session to exercise the real executable-relative log path. It returns 1 if resources
or logging fail. `make test-platform` includes a substituted path-discovery failure
fixture verifying that accepted moves retain an I/O diagnostic.

The GTK application keeps its entry point and shared private header in `apps/gtk/`.
Its `app/` directory owns lifecycle, commands and navigation; `screens/` owns pages;
`ui/` owns reusable presentation and messages; `async/` owns background search jobs.
Platform startup/clock helpers live in `src/platform/runtime/`, with logging in
`src/platform/logging/` and one private `platform.h` interface.

`mk/modules.mk` explicitly lists the owned source directories and discovers their
C files. Add a directory there when introducing a new application subsystem.
Resources are split between `assets/pieces/` and `assets/icons/`; aliases in the
manifest preserve their existing GResource names. Public core headers and the
rules/session/AI source and test boundaries remain unchanged.

Packaging scripts and runtime templates live in `tools/packaging/`, developer
checks/cleanup/benchmarks in `tools/dev/`, and historical probes in `tools/legacy/`.
Scripts locate the repository without requiring `.git`, including in source archives.
See the [documentation index](../README.md) for user, architecture, development
and historical documents.

## Working conventions

Public headers belong in `include/anteater`. Use `ac_` functions, `Ac` types, `AC_` constants. Application-private helpers may use `gui_`. Follow `.clang-format`, four spaces and the existing brace style. Prefer module-private helpers; expose operations that enforce invariants rather than internal transition machinery. Comments should explain a contract or reason. Keep gameplay definitions in rules, state/history in session, search caches in context, and widgets in GTK.

To add a module, declare its narrow interface, place implementation under the owning directory, and update [mk/modules.mk](../../mk/modules.mk) if adding a library. New `.c` files in an existing module are automatically discovered. Add `test_*.c` under `tests/rules`, `tests/session`, or `tests/ai`; Make discovers them. Use injected clocks and failure callbacks instead of sleeps. New special moves need generation, legality, apply/unmake, hash, request resolution and GUI regression coverage. Update the manual/spec when observable behavior/contracts change.

## Packages

```sh
make CONFIG=release package-source package
python3 tools/packaging/verify.py
```

`tar` aliases `package-source`; `tar-user` aliases `package`. Archives use `AnteaterChess-Reborn-<version>-source.tar.gz`, `...-windows-x64.zip`, or `...-linux-x64.tar.gz`, under `dist/`. The source package includes all development documents, historical PDFs, tests, Make modules and packaging tools, and can rebuild and repackage without Git. Binary packages contain the user manual, historical user PDF, COPYRIGHT and platform INSTALL instructions. Windows bundles recursively discovered non-system DLLs, GdkPixbuf SVG loader, schemas, icons, and third-party licenses/manifest. Linux uses system GTK 3.

The package script compares all relevant input bytes with a saved manifest, including
docs/templates/tools. Unchanged inputs reuse an archive; any input change rebuilds it.
SHA256SUMS covers distribution archives. Verification selects the host's runtime and
the source archive, extracts to a temporary directory with spaces and Chinese
characters, rebuilds core and desktop sources, and checks repackaging invalidation.
It verifies captured version output, concurrent and repeated sessions, executable-side
logs, Linux symlink launches and blocked log directories. Windows validation also
checks PE GUI/console subsystems and an independent console build. Runtime logs are
excluded from archives. Packaging tests operate on temporary copies.

## CI and formal release

The workflow builds/test/packages on Ubuntu 24.04 and Windows UCRT64. Linux runs ASan/UBSan and GTK tests under Xvfb. Artifacts include runtime archives, source, checksums and performance output. Use one exact commit for both platforms. To release: confirm both jobs passed, download artifacts, verify hashes, create a draft release for the version tag, upload source and both runtimes plus combined SHA256SUMS, then publish only when the full set is present. `tools/packaging/release.py` performs the artifact checks and draft-to-published transition using authenticated `gh`. A failed upload/check leaves the release draft.

No macOS release or automated push to the original repository is configured. COPYRIGHT is not replaced by a new license. See [validation](validation.md) for measured results and remaining limits.
