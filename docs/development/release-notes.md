# C++20 native migration candidate (unpublished)

All maintained core and desktop code uses C++20. Typed values, RAII Session/search
owners, independent policies and allocation errors replace the former C API.
Snapshots and requests own their history. Qt models have narrow notifications,
registered types and enums; ordinary ticks preserve focus and history scrolling.
Background work uses cooperative stop tokens and complete late-result checks.

CMake presets, CTest, install and CPack replace Make wrappers and custom archives.
Windows uses native v143 14.44, official Qt 6.11.2 and permitted Release runtime
sources; Linux retains Ubuntu 24.04 system Qt 6.4.2. Dependency/license inventories
and checksums accompany candidates. SVG/PDF/COPYRIGHT and frozen expectations
remain unchanged; one recorded resource alias adjustment removes WhiteAntsvg.

No AI tuning or variant rule change is included. Difficulty 4 remains invalid.
Previous reports are archived. See [the native ledger](native-migration/validation.md)
for measured coverage. Clean Windows and human acceptance remain pending when
their environment has not been provided; no Release is automatically published.
