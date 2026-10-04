# Windows x64 installation

Extract the entire ZIP to a writable directory and launch `anteater-chess.exe`.
The game opens without a console window; no MSYS2 installation is required.
Keep all DLLs and the lib/share directories with the executable. Embedded pieces
do not depend on the working directory. Game logs are created in `logs/` beside
the executable. Keep that directory when moving the program to retain its logs.
An unwritable log directory reports a diagnostic while game actions still apply.

THIRD-PARTY.md lists bundled dependency packages and corresponding source-package locations; licenses are in licenses/. The application itself retains COPYRIGHT's course-specific terms.
