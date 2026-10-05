# Windows x64 installation

Extract the complete CPack ZIP to a writable directory and launch anteater-chess.exe.
The game opens without a console. Keep its DLLs, plugins, qml and qt.conf together.
The package uses official Qt 6.11.2 MSVC 2022 x64 and v143 14.44 Release REDIST.
No development tool installation is required for the intended runtime.

Logs are in logs/ beside the actual executable, including with another working
directory. Concurrent instances use distinct files. Unwritable logs are reported
as diagnostics; accepted game actions remain applied.

DEPENDENCIES.json records sources, versions, dependency closures and hashes.
licenses/ contains Qt's vendor SBOM, license texts/attributions and Microsoft
notices. Qt shared libraries remain replaceable by compatible modified versions.
The original application COPYRIGHT remains unchanged.

SOURCE_REVISION binds this candidate to its source commit. FILES.sha256 covers
the archive payload. A local candidate is not independent clean Windows or
human desktop acceptance; consult the delivered validation attachments.
