# Archived original probe

legacy_probe.c is the original historical driver and is not compiled by the
maintained CMake graph. The original dual-engine fixture remains byte-identical
at tests/fixtures/probe.h. Active rule comparison is tests/rules/baseline_probe.hpp;
active AI comparison is tools/dev/ai_reference.cpp against the unchanged fixture.
