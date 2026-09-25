.DEFAULT_GOAL := all
include mk/config.mk
include mk/modules.mk
.PHONY: all gui headless tests test test-rules test-session test-ai test-gui run clean check help package package-source tar tar-user
all: gui
gui: $(APP)
headless: $(LIBS)
tests: $(TEST_BINS)
test: tests
	@set -e; for t in $(TEST_BINS); do echo "TEST $$t"; "$$t"; done
test-rules: $(filter $(BUILD)/tests/rules/%,$(TEST_BINS))
	@set -e; for t in $^; do "$$t"; done
test-session: $(filter $(BUILD)/tests/session/%,$(TEST_BINS))
	@set -e; for t in $^; do "$$t"; done
test-ai: $(filter $(BUILD)/tests/ai/%,$(TEST_BINS))
	@set -e; for t in $^; do "$$t"; done
test-gui: $(GUI_TEST)
	$(GUI_TEST)
run: gui
	$(APP)
$(BUILD)/obj/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
$(BUILD)/obj/apps/gtk/%.o: CPPFLAGS += $(GTK_CFLAGS) -Iapps/gtk -Isrc/platform
$(BUILD)/obj/src/platform/%.o: CPPFLAGS += $(GLIB_CFLAGS) -Isrc/platform
$(BUILD)/obj/tests/gtk/%.o: CPPFLAGS += $(GTK_CFLAGS) -Iapps/gtk -Isrc/platform
$(BUILD)/resources.c: assets/resources.xml $(wildcard assets/*.svg)
	@mkdir -p $(@D)
	glib-compile-resources $< --sourcedir=assets --generate-source --target=$@
$(BUILD)/resources.o: $(BUILD)/resources.c
	$(CC) $(CPPFLAGS) $(GTK_CFLAGS) $(CFLAGS) -c $< -o $@
$(BUILD)/lib/rules.a: $(RULE_OBJS)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^
$(BUILD)/lib/session.a: $(SESSION_OBJS)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^
$(BUILD)/lib/ai.a: $(AI_OBJS)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^
$(APP): $(GUI_OBJS) $(PLATFORM_OBJS) $(BUILD)/resources.o $(LIBS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(filter %.o,$^) $(GROUP_LIBS) $(GTK_LIBS) -o $@
$(BUILD)/tests/%$(EXE): $(BUILD)/obj/tests/%.o $(LIBS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $< $(GROUP_LIBS) -o $@
$(GUI_TEST): $(BUILD)/obj/tests/gtk/test_desktop.o $(filter-out %/main.o,$(GUI_OBJS)) $(PLATFORM_OBJS) $(BUILD)/resources.o $(LIBS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(filter %.o,$^) $(GROUP_LIBS) $(GTK_LIBS) -o $@
check:
	$(PYTHON) tools/check.py
$(BUILD)/benchmark$(EXE): $(BUILD)/obj/tools/benchmark.o $(LIBS)
	$(CC) $(LDFLAGS) $< $(GROUP_LIBS) -o $@
benchmark: $(BUILD)/benchmark$(EXE)
	$<
package-source tar:
	$(PYTHON) tools/package.py source
package tar-user: gui
	$(PYTHON) tools/package.py binary --build $(BUILD)
clean:
	$(PYTHON) tools/clean.py
help:
	@echo 'make [gui|headless|test|test-rules|test-session|test-ai|test-gui|run|check|package|package-source|clean]'
	@echo 'CONFIG=debug (default), release, or sanitize; CC, BUILD, CPPFLAGS, CFLAGS and LDFLAGS are overridable.'
-include $(shell find $(BUILD)/obj -name '*.d' 2>/dev/null)
.SECONDARY: $(TEST_OBJS)
