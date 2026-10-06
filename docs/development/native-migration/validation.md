# Native migration validation ledger

Baseline: `32da9759f3dfe1555de57b26c3d3b37ec4636f2a`.

## Current acceptance status

The user confirmed on 2026-10-05 that acceptance is complete, including the
independent clean-Windows and manual desktop gates. This closes the gates that
were pending in the earlier stage records below. The confirmation did not include
per-check observations, machine details, screenshots or separate acceptance
receipts; this ledger records the status as user-confirmed without adding results
that were not supplied. The verified software candidate remains commit
`cf2f30b7db09edac42cade7e7b61ad0db483d2a1`.

[The final remote run](https://github.com/Yide-Milo-Li/AnteaterChess-Reborn/actions/runs/37358903929)
passed both Windows and Linux jobs at that commit. The earlier
[published v2.0.1 release](https://github.com/Yide-Milo-Li/AnteaterChess-Reborn/releases/tag/v2.0.1)
is the earlier C11/GTK implementation at 6d92663. The native migration is integrated
into main. Version 2.1.0 is the native release line; see the
[release notes](../release-notes.md) for its delivery. Stage records below retain the
status and test counts measured at their respective stages; their historical
pending statements are superseded by this current status.

## Environment gate

Passed locally on 2026-10-05 before any tracked source edit. Windows Debug and
Release used MSVC 19.44.35228.0 (v143 14.44.35207 x64), Windows SDK 10.0.26100,
and official Qt 6.11.2 MSVC 2022 x64. Linux Debug and Release used Ubuntu 24.04,
GCC 13.3.0 and system Qt 6.4.2. Each probe compiled and ran C++20, pmr, span,
variant, stop_token, QML, SVG and a qsb shader. All four CTest runs passed 1/1.

[Machine-readable receipt](environment-gate.json) records exits and log hashes.
The complete private attachment directory is outside the repository at
`../migration-evidence/2026-10-05-native-migration`, including preserved earlier
failures. Repository build cleanup cannot delete it. Installer stdout is private
because it includes Qt account identifiers; redact identifiers for public delivery.

The first Qt install returned 0 while skipping a virtual Shader Tools child.
Installing its real parent `qt.qt6.6112.addons.qtshadertools` returned 0 and both
Windows combination probes then passed. An installer exit alone is not a gate.

## Stage status

| Stage | Status | Evidence |
| --- | --- | --- |
| Environment | Passed locally | Four combination probes; attachment hashes above |
| Build and conventions | Passed locally | Four desktop builds, 22/22 each; two core runs, 19/19 each |
| Rules and evaluation | Passed locally | Typed C++ API, semantic comparisons, injected rule-workspace failures; four 23/23 runs |
| Session ownership | Passed locally | Owning snapshots, resource lifetime, moves and progressive allocation failures; four 24/24 runs |
| Transaction and budget policy isolation | Passed locally | Independent policy module; Tournament and timeout failure matrix; four 25/25 runs |
| Search ownership | Passed locally | RAII contexts/requests, stop tokens, progressive failures and frozen output; four 26/26 runs |
| Desktop | Passed locally | Four 26/26 stage runs; calibrated native Windows/Xvfb checks verify actual DPR 1/1.5/2 |
| Distribution and cleanup | Passed locally | Nine presets; final Windows and Linux candidates and source archive verified |
| Remote CI | Passed | Windows and Linux jobs passed at `cf2f30b7db09edac42cade7e7b61ad0db483d2a1` |
| Clean Windows and manual acceptance | Complete per user confirmation | Confirmed 2026-10-05; detailed observations were not included with the confirmation |

Source migration, local candidate verification and external acceptance are recorded
as separate milestones. All are complete; no Release is published automatically.

## Protected data and driver changes

[The baseline inventory](protected-files.json) contains 27 SHA-256 records.
Historical fixtures, COPYRIGHT, PDFs and SVG bytes remain frozen. The dual-engine
`tests/fixtures/probe.h` is retained as historical evidence; the maintained driver
is extracted to `tests/rules/baseline_probe.hpp` for the C++ compiler. Its expected
strings still come from the unchanged `tests/fixtures/baseline.h`.

## Build-stage verification

Windows/Linux Debug and Release desktop configurations each passed 22/22 CTest
checks, including the unchanged 27-position AI score/SEE/search reference. Debug
core-only configurations each passed 19/19. Configuring Windows core-only with
both find_package calls explicitly disabled succeeded; no Qt or Python package
was discovered. Both Windows and Linux production core-only builds passed with Qt/Python discovery disabled. Linux ASan/UBSan passed 19/19 with leak detection enabled.
All configure/build/test child exit codes were 0 after repairing C-to-C++ casts;
failed first compile logs are retained separately in the private attachment tree.
Commands used the checked-in presets. Qt GUI tests used offscreen/software/Basic.
This establishes build compatibility of the existing algorithms, not the planned
owning Session/SearchRequest API or human desktop acceptance.
## Rules and evaluation verification

The C++ API uses namespace `ac`, `.hpp/.cpp`, scoped enums and fixed value arrays.
Status, allocation-free Error metadata and variant-based Result<T> are established.
No C linkage wrapper remains in maintained headers. Position and Move equality
compare fields and arrays, so apply/unmake checks no longer inspect padding bytes.
Rules temporary workspaces use RAII and an injected memory resource. Their failure
checks preserve caller outputs and release every allocated block.

Windows/Linux Debug and Release each passed 23/23, including the immutable AI
score/SEE/legal-move/chosen-move/depth/node reference. Linux ASan/UBSan passed
20/20. These tests still exercise the transitional Session/search owners, which
are replaced in the following stages. All 27 protected files remain unchanged.

Localized MSVC header dependency detection initially produced a mojibake prefix
and zero recorded dependencies. Requesting English diagnostics was insufficient
because this toolset only has Chinese language resources. The CMake configuration
now detects /showIncludes using Ninja's actual encoding, then a clean native
rebuild passed. `ninja -t deps` records rules.hpp and types.hpp (two valid direct
project dependencies) for board.cpp. Raw failed and successful receipts remain
outside the disposable build tree. This adjustment does not modify installed MSVC.

## Session ownership verification

Session is noncopyable and movable. State polling allocates nothing. Snapshot
history and hashes remain valid across mutation, undo, restart and destruction.
Progressive failure injection checks all construction, snapshot and human move
preparation allocations, no partial publication and complete resource cleanup.
Move construction/assignment, including assignment across resources, allocates
nothing. Desktop logging now follows committed state; a failed path preserves an
accepted move and displays a separate diagnostic.

Windows/Linux Debug and Release passed 24/24. Linux ASan/UBSan passed 21/21.
All frozen rule/AI comparisons pass and all 27 protected files remain unchanged.
Raw failures and successful commands/hashes are retained in stage3 attachments.
MSVC Debug initially terminated during snapshot failure injection: the installed
standard library allocates an iterator proxy inside its noexcept vector allocator
constructor. Exact-sized resource-owned sequences now avoid that hidden allocation
without disabling Debug iterator checks. The repaired Debug ownership test and
full matrix pass. Search ownership and narrow desktop model migration remain pending.

## Transaction and policy verification

Configuration, search-depth selection and Tournament budgets live in `ac_policy`.
Session links Rules and Policy; it has no AI library dependency. Search receives
explicit limits. Tournament charges prepare a value before move preparation and
publish it only with an accepted move. Progressive AI preparation failures preserve
position, history, hashes, revision and balances. Undo does not refund charged time.
Promotion allocation errors preserve state. Timeout still ticks before rejecting
a stale command, without allocating. Desktop snapshot allocation errors remain
diagnostic after an accepted start.

Windows/Linux Debug and Release passed 25/25, Linux ASan/UBSan 22/22.
A separate UBSan probe exposed preexisting signed overflow when adding an INT_MAX
configured budget to a nonempty saved-time pool. Wide integer arithmetic now
applies the same retained cap without overflow; boundary regression and frozen
comparisons pass. Failed probe and repaired checks are separate attachments.
Unattended Windows tests direct CRT diagnostics to stderr and have bounded timeouts.

## Search ownership verification

SearchContext and SearchRequest own all required workspaces, position and hash
inputs. Both are noncopyable and movable. Progressive resource failures cover
every factory allocation and release partial state. Requests survive Session and
snapshot destruction; context/request moves preserve ownership without allocating.
Synchronous searches allocate nothing and retain no borrowed input spans afterward.
Cancellation before and during search preserves the caller's position.

Qt task construction and worker workspace failures produce explicit errors. Tests
exercise every task resource allocation, complete cleanup after shutdown, matching
gameId/revision/generation, and failure suppression across 100 ticks followed by a
successful new-game retry. Existing move/restart/undo/timeout/navigation/hint/close
and queued-completion lifecycle tests pass. QThread tasks use stop_source/stop_token.
Windows/Linux Debug and Release passed 26/26, Linux ASan/UBSan 23/23, with all
immutable score/SEE/legal/chosen/depth/node references unchanged on both compilers.
Desktop model/type registration and distribution remain pending.

## Desktop verification

ApplicationController owns Session and publishes separate registered settings,
input, board, history, clock and status models. QML uses required typed properties
and named enums. The CMake QML module generates registration, resources and its
import plugin; the retired hand-written QML manifest is archived in attachments.
Ordinary 100 ms ticks emit only changed clock values, allocate no Session data,
and preserve input focus and history position. A deferred follow-end callback now
checks the reader's current scroll policy before repositioning the history.

Windows/Linux Debug and Release passed 26/26. Six additional desktop runs passed
at scale factors 1, 1.5 and 2, with software rendering, the native Windows backend
and Linux Xvfb/xcb. Screenshots include menus, setup, gameplay, promotion,
confirmation, fullscreen and endgame. Native screenshots were visually inspected.
Windows offscreen screenshots had missing font glyphs and are retained as failed
visual evidence, not accepted screenshots. Tests cover maximized and ordinary
F11/Escape restoration, input, promotion, focus, scrolling and task lifecycles.
All 27 protected files remain unchanged. These automated local checks do not
replace independent clean Windows or human desktop acceptance.

## Distribution, cleanup and final matrix

The final maintained comparator is a single C++ executable reading the original
AI fixture. Retiring the duplicate Python packaging test changes the final CTest
count to 25 desktop and 22 core. Windows/Linux Debug and Release desktop each
passed 25/25; all four core presets and Linux ASan/UBSan passed 22/22. Production
core builds also passed with Qt/Python discovery explicitly disabled. New atomic
log replacement failures preserve existing bytes and recover on the next write.

The checked-in CMake install graph and CPack configurations replace custom
archivers. Windows installation uses Qt's official QML deployment API and validates
90 deployed x64 binary closures against official Qt, v143 14.44 Release REDIST
and permitted SDK D3D REDIST origins. Versions, hashes and licenses are recorded.
Linux installation independently validates 37 executable/plugin/QML ELF closures,
system package versions, QML modules and license records; Qt 6.4's library deployment
limitation is explicit. Prototype install logs are retained; they are not formal
clean-commit candidates. Final candidate receipts must identify one clean commit.

Repeated native initialization initially accumulated PATH beyond cmd's limit.
The script now restores its original process inputs; six successive initializations
produce a stable 3790-character PATH and the same compiler/Release REDIST locally.
The interrupted Release core preset then passed. No machine PATH was changed.

Earlier scale-factor-only screenshots did not prove effective 100/150/200%:
this Windows display reports native DPR 2. The calibrated helper first measures
the window, then checks eight page captures at actual DPR 1, 1.5 and 2. Both
platforms passed all three 18-case software-rendered desktop runs. Captures were
inspected locally; human desktop acceptance remains pending. Earlier empty Windows
QtTest logs came from shell argument parsing and are insufficient evidence; new
runs use an absolute QtTest report argument and separate console logs.

Only the authorized resource alias adjustment changes a protected file:
[structural-changes.json](structural-changes.json) records before/after hashes of
assets/resources.qrc. All 26 other protected files remain byte-identical, including
SVGs, PDFs, COPYRIGHT and fixtures. .gitattributes preserves their original Windows
checkout CRLF bytes on both platforms while maintained source uses LF. Frozen
expected values are unchanged. Historical probes and migration reports are archived
with applicability notes; old candidates/cache inputs are preserved outside dist.

Commands, real exits and log/screenshot hashes are in stage7-final, stage7-final-r2,
stage7-production-core, stage7-repeated-environment and stage7-calibrated attachment
receipts. The external CURRENT.md and final artifact index will record formal
candidate and remote CI outcomes after this source commit; this avoids changing
the tested commit merely to embed its own generated checksums. Clean Windows and
manual acceptance remain separate open gates. No Release is published.

## Candidate and CI repair evidence

The first clean-commit candidate pass stopped at Windows extraction because the
verification directory nested long license filenames beyond the normal Windows
path limit. The verifier now strips the known CPack root into short space/Unicode
working names and accepts a separate bounded work directory. No system long-path
or display configuration was changed. Native portable startup, concurrent logs,
blocked logs and missing-DLL detection then passed.

The Windows no-Git rebuild next exposed Qt 6.11's import-scanner response-file
encoding: CMake writes UTF-8; Qt reads those lines using the local ANSI code page.
A CMake-built wide-command-line adapter preserves the original official tool,
public import/deployment graph and real child exit. Configure and the complete
151-step Unicode source build passed with that repair. Final untouched source
archives are rebuilt again after committing the adapter. Linux candidate startup,
symlink placement and the canonical Windows-generated no-Git source archive
already passed all 25 tests and the benchmark locally at the preceding commit.

Remote Linux CI passed at commits 36dd983 and 53df4b2. Remote Windows first stopped
on an unsupported installer --wait argument (87); setup.exe now uses PowerShell's
process wait. The next attempt reached Qt installation, where aqt 3.3 could not
resolve Qt 6.11's new kit-specific repository hierarchy. CI now reads the pinned
official metadata directly, verifies archive checksums and records extraction
results. Failed attempts remain attachments. Remote Windows and Linux CI passed for
the final tested commit; their receipts and logs are included in the delivery.

The b03a39a remote Windows job subsequently passed all four build/test presets,
three effective DPI runs, official deployment and portable startup checks. Its
Unicode no-Git rebuild reached CTest and failed there. A local path containing a
chess glyph reproduced the AI comparison driver's narrow-argv limitation (exit 2).
The driver now uses wmain and filesystem's native wide path on Windows; the same
unchanged fixture then passed all 27 comparisons (exit 0). Linux retains its native
UTF-8 main. Final candidates and CI are rerun at the repair commit. Candidate
command logs are included in CI artifacts so failures can be inspected directly.

## Documentation alignment on 2026-10-05

README, the documentation index, manual, specification, development/CI guides,
release notes, migration-plan status and runtime templates were reviewed against
the maintained code, CMake presets/install graph, package receipts and GitHub run
37358903929. Local main contains cf2f30b; remote main was verified at 6d92663.
The older published v2.0.1 release still contains C11/GTK. No application source,
rule, AI expectation or historical report was changed in this documentation pass.

The interactive diagram now keeps QThread inside the Qt desktop boundary,
identifies the independent budget policy, and shows only the injected clock
entering Session. Logs remain a desktop effect after accepted commands.
Archify's final showcase validation/delivery passed all nine checks with zero
errors/warnings. Visual containment passed at 1440×900, 1600×1000, 1920×1080 and
2048×1320; light/dark captures at the smallest/largest sizes were inspected.

Windows checks used v143 14.44.35207 x64, Qt 6.11.2 MSVC 2022 x64, native
CMake 4.3/Ninja and Python 3.14. The source code remains at cf2f30b with uncommitted
documentation changes; configuration identifies this checkout as `-dirty`.

| Check | Command or method | Result |
| --- | --- | --- |
| Documentation/resources/layout | `python tools/dev/check.py` | Exit 0 |
| Whitespace | `git diff --check` | Exit 0 |
| Native Release configuration | `cmake --preset windows-release` | Exit 0 |
| Source CPack | `cpack --config build/windows-release/CPackSourceConfig.cmake -B ../migration-evidence/dr5/source` | Exit 0 |
| Windows runtime CPack | `cpack --config build/windows-release/CPackConfig.cmake -B ../migration-evidence/dr5/runtime` | Exit 0; 90 binary closures verified |
| Runtime documentation payload | Compare archived README, INSTALL, manual, archive index and PDFs with checkout bytes | Six files match; 14 maintained relative links resolve |
| Protected data | SHA-256 against protected-files.json and the authorized structural adjustment | 27/27 match; four archived-report hashes also match |

Raw command/exit/hash receipts and diagram captures are retained outside build
at `migration-evidence/2026-10-05-documentation-review/` beside the repository.
Diagram specification SHA-256:
`b936daad8d5397d211f789bbfbcb770129a582e2a45e4dc7b970e035128d6c1d`.
Delivered HTML SHA-256:
`2fba1bd6e625053a4e9a484d4edb033e9352d86aed61a77602cf3d5e0071e9d1`.
The generated check packages are dirty-checkout documentation evidence, not new
formal candidates. Original accepted archive hashes remain unchanged in their
delivery. Application CTest and desktop interaction tests were not rerun for this
documentation pass; the existing code results above remain bound to cf2f30b.

## Version 2.1.0 release preparation — 2026-10-05

The user authorized a new release and repository-homepage refurbishment.
VERSION is now 2.1.0. README includes direct platform downloads, current variant
rules, documentation links and native screenshots. Repository presentation and
maintained package/version descriptions were refreshed; game source, frozen
fixtures, COPYRIGHT and historical PDFs were not edited.

Screenshots were generated by the existing `renderedPagesAndFocus` QtTest slot
from Windows Release, MSVC v143 14.44.35207 x64 / Qt 6.11.2. It passed (3 QtTest
entries, exit 0), including focused input through clock updates, fullscreen,
promotion cancel and end-game confirmation. Eight captures reported actual
DPR 1.5. Four unmodified PNGs are tracked under docs/media. Raw screenshots and
the QtTest receipt are retained outside build at
`../migration-evidence/release-2.1.0/`. An initial attempt failed to open its
report because the evidence directory did not yet exist; after creating that
directory, the command completed successfully.

Release packages require a clean commit and a successful complete Windows/Linux
CI run for that commit. CI artifacts contain command exits, archive checksums,
candidate-verifier results and exact source identity. Public release assets are
copied from that run; source-package identity is checked on both platforms.

## Version 2.1.1 release preparation — 2026-10-05

Resolved Anteater multi-path ambiguity in `src/rules/resolver.cpp` and `src/rules/validation.cpp`.
Coordinate input now automatically prioritizes direct 1-step capture for adjacent targets and greedy
maximal capture for distant targets. All 25/25 CTest suite tests passed. VERSION is now 2.1.1.
