# Native migration validation ledger

Baseline: `32da9759f3dfe1555de57b26c3d3b37ec4636f2a`.

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
| Transaction and budget policy isolation | Pending | Tournament rollback and Session-to-rules dependency cutover |
| Search ownership | Pending | Owned requests, cancellation and frozen AI output |
| Desktop | Pending | Narrow models, lifecycle and interaction checks |
| Distribution and cleanup | Pending | CPack, dependency and license manifests |
| Remote CI | Not run | Must be bound to the final tested commit |
| Clean Windows and manual acceptance | Pending | No new VM is provisioned |

Source migration, local candidate verification and external acceptance are separate
milestones. No release is published automatically.

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
