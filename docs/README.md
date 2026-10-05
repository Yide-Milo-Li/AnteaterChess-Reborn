# Documentation

Current documentation describes the C++20 / Qt 6 native implementation. It was
reviewed on 2026-10-05 against source commit `cf2f30b`. VERSION remains `2.0.1`,
which is also used by the older published C11/GTK release; use source revisions
to distinguish them.

## Playing the game

- [User manual](user/manual.md): variant rules, controls, timing, undo and logs.
- [Windows package setup](../tools/packaging/templates/INSTALL-windows.md) and [Ubuntu package setup](../tools/packaging/templates/INSTALL-linux.md): source templates delivered as INSTALL.md in native archives.

## Developing and maintaining

- [Software specification](architecture/software-specification.md): current module boundaries, ownership, lifecycle and errors.
- [Session ownership](architecture/session-ownership.md) and [search ownership](architecture/search-ownership.md): allocation, lifetime and transaction contracts.
- [Interactive architecture](architecture/architecture.html), with its [JSON source](architecture/architecture.json): current components and relationships.
- [Development guide](development/guide.md): toolchains, presets, checks, installation and CPack.
- [CI guide](development/ci.md): the Windows/Linux workflow and its measured status.
- [Release notes](development/release-notes.md): native migration changes and publication status.
- [Agent instructions](../AGENTS.md): repository constraints.

## Migration evidence

- [Native validation ledger](development/native-migration/validation.md): the authority for engineering and acceptance status, bound to the tested source commit.
- [Native migration plan](development/native-migration/plan.md): the implemented scope and original verification requirements.
- [Migration map](development/migration.md): retired interfaces and their maintained replacements.
- [Test migration inventory](development/test-migration.md): original scenarios and current regression coverage.
- [Protected-file inventory](development/native-migration/protected-files.json) and [structural changes](development/native-migration/structural-changes.json): frozen data and the authorized resource-alias change.

The ledger records local packages and remote CI as passed, and clean-Windows/manual
acceptance as complete per user confirmation. Detailed independent acceptance
receipts were not supplied. The native migration is integrated into main.
No native Release is published.

## Historical material

[Archive policy and inventory](legacy/README.md) identify the original course PDFs,
obsolete migration reports and the retired probe. They are historical evidence and
receive no ongoing behavior updates. Current Markdown manuals and contracts are
authoritative. The small forwarding files at older development-document paths
exist only to direct readers to the native ledger or historical archive.
