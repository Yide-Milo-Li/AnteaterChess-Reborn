# Historical archive

These files preserve earlier submissions and implementations. They receive no
ongoing behavior updates. The current [user manual](../user/manual.md) defines the
game's rules and controls. In a source checkout, current engineering contracts and
acceptance evidence are in `docs/architecture/software-specification.md` and
`docs/development/native-migration/validation.md`.

| Material | Purpose | Retention |
| --- | --- | --- |
| [Course user manual](Chess_UserManual.pdf) and [software specification](Chess_SoftwareSpec.pdf) | Original EECS 22L submissions and authorship history | Preserve original bytes under AGENTS.md and the protected-file inventory. |
| [Earlier migration reports](migrations/README.md) and migrations/inventory.json | Findings and limitations for retired C11/GTK and C11/C++17 implementations | Read-only archive; superseded by the native ledger. |
| [Original probe](probes/README.md) and probes/legacy_probe.c | Provenance for the retired comparison driver | Uncompiled archive; maintained C++ drivers compare the frozen fixtures directly. |

## Retirement boundaries

Old migration Markdown and forwarding pages can be removed in a separate cleanup
once their links, archive inventory, packaging graph and development checks are
updated together. Preserve their historical content in Git rather than rewriting
old claims to describe new behavior.

Deleting the entire legacy directory would also remove protected course PDFs.
The original COPYRIGHT, attribution and frozen regression data must remain.
Files under `tests/fixtures/`, including names containing legacy or probe, are
still regression evidence for maintained tests; archival naming is not permission
to delete or regenerate them.

The runtime install graph currently includes this archive. Excluding obsolete
reports/probe sources from runtime packages can be considered independently of
retaining them in the source repository.
