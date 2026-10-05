# Native migration: accepted, unpublished

Reviewed on 2026-10-05. VERSION remains `2.0.1`, while the native implementation
is identified by source commit `cf2f30b7db09edac42cade7e7b61ad0db483d2a1` in
`main`. The
[published v2.0.1 release](https://github.com/Yide-Milo-Li/AnteaterChess-Reborn/releases/tag/v2.0.1)
at `6d92663` contains the earlier C11/GTK implementation. No native Release is published.

## Changes

All maintained core and desktop code uses C++20. Scoped values, RAII Session/search
owners, independent configuration/budget policies and typed allocation errors
replace the former C API. Snapshots and search requests own their history.
No C ABI or compatibility wrapper is maintained.

Qt 6/QML replaces GTK. ApplicationController owns the live Session; registered
models provide narrow notifications. Ordinary ticks update clocks without
replacing pages, input focus or history scroll state. Background QThread jobs own
their requests, use cooperative stop tokens and validate late-result identities.
SVG rasterization follows board geometry and the display pixel ratio.

CMake presets, CTest, install and CPack replace Make wrappers and custom archives.
Windows uses native v143 14.44 x64, official Qt 6.11.2 MSVC 2022 x64 and permitted
Release runtime sources. Ubuntu 24.04 uses GCC and system Qt 6.4.2. Runtime/source
archives include source identity and checksums; runtime archives also include
dependency and license inventories. Unicode Windows source rebuilds use adapters
for Qt import scanning and native fixture paths.

Variant rules, undo/timeout policies, Tournament allocation/no-refund and AI-only
repetition are preserved. Frozen AI score/SEE/move/depth/node expectations are
unchanged. Experimental difficulty is removed; numeric value 4 is rejected.

COPYRIGHT, attribution, historical PDFs, SVGs and frozen fixtures retain their
bytes. The authorized WhiteAnt resource alias adjustment is recorded separately.
Previous implementation reports are archived; see the
[archive policy](../legacy/README.md).

## Verification and acceptance

Local final runs passed 25/25 checks for each of four desktop presets and 22/22
for each of four core presets and Linux ASan/UBSan. Windows and Linux candidates
and no-Git source rebuilds passed for `cf2f30b`.
[Remote Windows/Linux CI](https://github.com/Yide-Milo-Li/AnteaterChess-Reborn/actions/runs/37358903929)
passed for the same commit.

Clean-Windows and manual desktop acceptance are complete per the user's
2026-10-05 confirmation. Individual machine details, screenshots and per-check
receipts were not supplied with that confirmation. The
[native ledger](native-migration/validation.md) preserves this distinction and the
earlier stage records. Publication remains a separate step.
