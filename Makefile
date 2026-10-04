.DEFAULT_GOAL := all
PYTHON ?= python3
CONFIG ?= debug
BUILD ?=
WINDOWS_CONSOLE ?= 0
GOALS := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),all)
COMMANDS := all gui headless tests test test-rules test-session test-ai test-platform test-gui run clean check help package package-source tar tar-user benchmark
.PHONY: $(COMMANDS)
# Dispatch once even for `make -j4 test gui`: CMake owns the dependency graph.
$(filter-out $(firstword $(GOALS)),$(COMMANDS)):
	@:
$(firstword $(GOALS)):
	$(PYTHON) tools/dev/build.py --config $(CONFIG) --build "$(BUILD)" --console $(WINDOWS_CONSOLE) $(GOALS)
