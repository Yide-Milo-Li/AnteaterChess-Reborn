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

SOURCE_REVISION identifies this archive's source commit; VERSION alone does not
distinguish it from the older published C11/GTK release. FILES.sha256 covers the
archive payload. Package-verification receipts establish local deployment checks.
Clean-Windows and human desktop acceptance are recorded separately in the
delivered validation ledger, with their evidence source identified.
