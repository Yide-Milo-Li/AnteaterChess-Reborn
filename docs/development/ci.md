# Continuous integration

.github/workflows/ci.yml tests the pushed commit with native Windows v143 14.44
and official Qt 6.11.2 MSVC 2022 x64 (including Shader Tools), and Ubuntu 24.04
GCC/system Qt 6.4.2. Windows installs only a missing pinned component and refuses
nonzero installer results, including restart-required 3010. Each job initializes
its own process environment; developer paths are cleared for candidate startup.

Debug/Release desktop/core and Linux ASan/UBSan are separate presets. GUI checks
also run with native Windows or Xvfb/xcb software rendering at 100/150/200%.
CMake install, CPack and the candidate verifier produce runtime/source artifacts,
dependency/license inventories, checksums and command/exit/hash receipts. Rebuild
verification extracts source without Git into a path containing spaces/Unicode.

Remote CI is measured only after the named run completes for the final commit.
It does not replace a clean Windows host or human interaction acceptance.
No workflow publishes a Release; publication remains a separate decision.
