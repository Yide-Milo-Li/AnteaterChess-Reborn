# Layout and desktop runtime validation

Date: 2026-10-04. Baseline: `6d92663b4899bab6c0b8a1da021b739032ef26f1`.
These are local implementation checks, not a new published release or a new CI run.
The existing version number is retained for the locally generated archives.

## Environments and baseline

- Windows x64: MSYS2 UCRT64, GCC 16.2.0, GTK 3.24.52 and GLib 2.90.0.
  The missing UCRT64 packages were installed under the existing `C:/msys64`.
- Linux: Ubuntu 24.04.4 under WSL2, GCC 13.3.0, GTK 3.24.41 and GLib 2.80.0.
  Desktop tests ran under Xvfb; this is not a native Ubuntu desktop visual review.
- An independent archive of the baseline source passed all 15 core executables,
  Release GUI compilation, documentation/resource checks and GTK integration on
  both environments before evaluating the changed implementation.

## Automated results

| Check | Result |
| --- | --- |
| Core rules, session and AI regression executables | 15/15 passed on both platforms; core public headers and implementations have no changes |
| Release GTK integration | Passed on both platforms: mode navigation, typed moves, undo, hints, AI turns, game replacement and shutdown during search |
| Resource aliases and scaling | All 14 pieces and 4 icons loaded at 32, 96 and 160 pixels |
| Platform adapters | Both fixtures passed: explicit/default paths, independent instances, game rotation, snapshot replacement and accepted moves after unavailable path discovery |
| Linux write permissions | An unprivileged fixture on the native temporary filesystem rejected writes to a chmod 0500 directory without creating a log |
| Linux ASan/UBSan | Core and platform suites passed with leak detection; GTK suite passed with leak detection disabled for GTK global caches |
| Windows subsystems | Release and Debug game EXEs are GUI (2); console Debug game and test EXEs are Console (3); source-package verification also checked Release-console isolation |
| Source and runtime packages | Host-specific verification passed on Windows and Linux; core and desktop source rebuilds, archive invalidation, checksums and PDF copies passed |
| Portable runtime logs | Space/Chinese extraction paths, another working directory, concurrent processes and repeated sessions passed; Linux symlink launch wrote beside the actual executable |
| Blocked log directory | Both runtime packages reported smoke-test failure when a regular file occupied logs/, without creating working-directory logs |
| Captured version output | GUI Windows and Linux runtime processes returned the expected version on stdout with exit code 0 |
| Windows shell launch | Direct EXE and .lnk shell-open routes displayed the Anteater Chess window; a detached probe confirmed neither game process owned a console |

The Windows shell check launched the packaged executable in a path containing spaces
and Chinese characters, and a shortcut with a different working directory. Its
console probe detached itself before calling AttachConsole on the game PID; both
attempts failed with ERROR_INVALID_HANDLE (6), meaning the target had no console.
Only the test-created game windows were closed after checking.

## Human acceptance and publication

Human double-click/shortcut observation and an exhaustive visual review are **not
recorded**. The shell-open checks above are automated OS-level checks and remain
separate from human acceptance. Current GitHub CI and formal release publication
have not been claimed for these local commits.

Local raw logs and the Windows shell probe are under `build/`; generated archives
and SHA256SUMS are under `dist/`. `make clean` removes these generated artifacts.
The final evidence-only documentation update is followed by repackaging and archive
integrity checks; it does not change the validated runtime executable.

## Historical PDF integrity

- `docs/legacy/Chess_SoftwareSpec.pdf`: SHA256 `7ccec2f1c3a82cefeca4e053ebfb20a9c2774b6888901e22757c4fb53aa7d8fe`.
- `docs/legacy/Chess_UserManual.pdf`: SHA256 `60ce3a20325c93e24a97a3e8eb5345b3261a4a6b371c395c982800ce4035e4ee`.

Both match the baseline. COPYRIGHT and team attribution are retained. Earlier
user-directory logs remain in place; new games use the executable-side directory.
