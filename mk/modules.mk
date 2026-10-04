RULE_SRCS := $(wildcard src/rules/*.c)
SESSION_SRCS := $(wildcard src/session/*.c)
AI_SRCS := $(wildcard src/ai/*.c)
GUI_DIRS := apps/gtk apps/gtk/app apps/gtk/screens apps/gtk/ui apps/gtk/async
PLATFORM_DIRS := src/platform/runtime src/platform/logging
GUI_SRCS := $(foreach directory,$(GUI_DIRS),$(wildcard $(directory)/*.c))
PLATFORM_SRCS := $(foreach directory,$(PLATFORM_DIRS),$(wildcard $(directory)/*.c))
RULE_OBJS := $(RULE_SRCS:%.c=$(BUILD)/obj/%.o)
SESSION_OBJS := $(SESSION_SRCS:%.c=$(BUILD)/obj/%.o)
AI_OBJS := $(AI_SRCS:%.c=$(BUILD)/obj/%.o)
GUI_OBJS := $(GUI_SRCS:%.c=$(BUILD)/obj/%.o)
PLATFORM_OBJS := $(PLATFORM_SRCS:%.c=$(BUILD)/obj/%.o)
LIBS := $(BUILD)/lib/session.a $(BUILD)/lib/ai.a $(BUILD)/lib/rules.a
GROUP_LIBS := -Wl,--start-group $(LIBS) -Wl,--end-group
TEST_SRCS := $(wildcard tests/rules/*.c tests/session/*.c tests/ai/*.c)
TEST_OBJS := $(TEST_SRCS:%.c=$(BUILD)/obj/%.o)
TEST_BINS := $(TEST_SRCS:tests/%.c=$(BUILD)/tests/%$(EXE))
APP := $(BUILD)/bin/anteater-chess$(EXE)
GUI_TEST := $(BUILD)/tests/gtk/test_desktop$(EXE)
PLATFORM_TESTS := $(BUILD)/tests/platform/test_runtime$(EXE) $(BUILD)/tests/platform/test_path_failure$(EXE)
