# Frozen baseline data

BASELINE, baseline.h, ai-evaluation-baseline.txt, legacy-scenarios.json and probe.h
are immutable evidence. probe.h is archived in place to preserve its original
path and bytes; no maintained target compiles its legacy engine branch.
The historical executable driver is under docs/legacy/probes.

The maintained C++ rule and AI drivers compare this data directly. A difference
must be investigated; changing expected values is not an accepted migration fix.
