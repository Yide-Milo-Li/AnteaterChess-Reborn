# Development and release

## Toolchain

Use C11, GNU Make, GCC, pkg-config, Python 3 and GTK 3 development packages. [README](../README.md) lists Windows UCRT64 and Ubuntu 24.04 setup commands. Use the UCRT64 shell, not the MSYS or MINGW64 environment. Runtime GUI adapters also require GLib >= 2.72. `glib-compile-resources` is supplied with GLib. Windows packaging needs `objdump`, `pacman`, GdkPixbuf loaders and their licenses.

## Build and test

```sh
make -j4 CONFIG=release
make test                    # core, no GTK required
make test-rules
make test-session
make test-ai
make test-gui                # requires a desktop/display
make check                   # documentation/resources/layout checks
make CONFIG=sanitize test    # Linux: ASan + UBSan
xvfb-run -a make test-gui     # Linux headless desktop automation
```

Core-only: `make headless test GTK_CFLAGS= GTK_LIBS= GLIB_CFLAGS=`. Targets do not evaluate GTK variables unless needed. Outputs live in `build/<platform>-x64/<configuration>/`; Debug, Release, and Sanitize do not share objects. GCC dependency files track included headers; the resource target depends on its manifest and SVG files. Override `CC`, `AR`, `CPPFLAGS`, `CFLAGS`, `LDFLAGS`, `BUILD` as needed; retain required language/include options when overriding flags. `make clean` removes only the checked `build` and `dist` generated directories.

Tests use `assert`, including in Release, so do not define `NDEBUG`. GUI tests invoke widget handlers and the GTK loop; interactive visual quality still requires human review. Sanitizer GUI runs disable leak detection for process-global GTK caches, while core sanitizer tests retain it. Each test is a separate executable; a failing command stops the target.

## Working conventions

Public headers belong in `include/anteater`. Use `ac_` functions, `Ac` types, `AC_` constants. Application-private helpers may use `gui_`. Follow `.clang-format`, four spaces and the existing brace style. Prefer module-private helpers; expose operations that enforce invariants rather than internal transition machinery. Comments should explain a contract or reason. Keep gameplay definitions in rules, state/history in session, search caches in context, and widgets in GTK.

To add a module, declare its narrow interface, place implementation under the owning directory, and update [mk/modules.mk](../mk/modules.mk) if adding a library. New `.c` files in an existing module are automatically discovered. Add `test_*.c` under `tests/rules`, `tests/session`, or `tests/ai`; Make discovers them. Use injected clocks and failure callbacks instead of sleeps. New special moves need generation, legality, apply/unmake, hash, request resolution and GUI regression coverage. Update the manual/spec when observable behavior/contracts change.

## Packages

```sh
make CONFIG=release package-source package
python3 tools/verify.py
```

`tar` aliases `package-source`; `tar-user` aliases `package`. Archives use `AnteaterChess-Reborn-<version>-source.tar.gz`, `...-windows-x64.zip`, or `...-linux-x64.tar.gz`, under `dist/`. The source package includes all development documents, historical PDFs, tests, Make modules and packaging tools, and can rebuild and repackage without Git. Binary packages contain the user manual, historical user PDF, COPYRIGHT and platform INSTALL instructions. Windows bundles recursively discovered non-system DLLs, GdkPixbuf SVG loader, schemas, icons, and third-party licenses/manifest. Linux uses system GTK 3.

The package script compares all relevant input bytes with a saved manifest, including docs/templates/tools. Unchanged inputs reuse an archive; any input change rebuilds it. SHA256SUMS covers distribution archives. Verification extracts to a temporary directory with spaces and non-ASCII characters, checks file lists/links, starts the program from another directory, rebuilds the source package and repackages it. Packaging tests operate on temporary copies.

## CI and formal release

The workflow builds/test/packages on Ubuntu 24.04 and Windows UCRT64. Linux runs ASan/UBSan and GTK tests under Xvfb. Artifacts include runtime archives, source, checksums and performance output. Use one exact commit for both platforms. To release: confirm both jobs passed, download artifacts, verify hashes, create a draft release for the version tag, upload source and both runtimes plus combined SHA256SUMS, then publish only when the full set is present. `tools/release.py` performs the artifact checks and draft-to-published transition using authenticated `gh`. A failed upload/check leaves the release draft.

No macOS release or automated push to the original repository is configured. COPYRIGHT is not replaced by a new license. See [validation](VALIDATION.md) for measured results and remaining limits.
