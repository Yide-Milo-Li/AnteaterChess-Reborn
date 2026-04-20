#
# Anteater Chess build configuration
#
# Alignment assumptions for future extensions:
# - Header files remain the truth source for what is considered public API.
# - The CLI executable is the only runnable frontend intentionally built here.
# - GUI sources remain excluded until their runtime contracts are implemented.
# - AI is built only where it is actually needed: the AI-specific test target.
#

.DEFAULT_GOAL := all

CC ?= gcc
EXE ?= .exe
CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Werror
DEPFLAGS := -MMD -MP
KEEP_DEPS ?= 1

RM ?= rm -f
RMDIR ?= rm -rf

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := bin
TEST_BIN_DIR := $(BIN_DIR)/tests

CLI_APP_NAME := anteater_chess_cli
CLI_APP_BIN := $(BIN_DIR)/$(CLI_APP_NAME)$(EXE)

CORE_SRCS := \
	src/core/position.c \
	src/core/piece.c \
	src/core/board.c \
	src/core/player.c \
	src/core/move.c \
	src/core/movelist.c \
	src/core/gameconfig.c \
	src/core/gamestate.c

GAMEPLAY_SRCS := \
	src/gameplay/execution.c \
	src/gameplay/endgame.c \
	src/gameplay/movegen.c \
	src/gameplay/rules.c

INPUT_SRCS := \
	src/input/command.c \
	src/input/command_parser.c \
	src/input/input.c

SYSTEM_SRCS := \
	src/event/event.c \
	src/event/event_queue.c \
	src/control/controller.c \
	src/control/fsm.c

SERVICE_SRCS := \
	src/error/error.c \
	src/log/log.c \
	src/time/clock.c \
	src/turn/turn.c \
	src/turn/turn_timer.c

CLI_SRCS := \
	src/cli/cli_feedback.c \
	src/cli/cli_renderer.c \
	src/cli/cli_menu.c \
	src/cli/cli_gameplay.c \
	src/cli/cli_app.c

AI_SRCS := \
	src/ai/ai.c

CLI_MAIN_SRC := src/main_cli.c

COMMON_SRCS := \
	$(CORE_SRCS) \
	$(GAMEPLAY_SRCS) \
	$(INPUT_SRCS) \
	$(SYSTEM_SRCS) \
	$(SERVICE_SRCS) \
	$(CLI_SRCS)

COMMON_OBJS := $(COMMON_SRCS:src/%.c=$(OBJ_DIR)/%.o)
AI_OBJS := $(AI_SRCS:src/%.c=$(OBJ_DIR)/%.o)
CLI_MAIN_OBJ := $(CLI_MAIN_SRC:src/%.c=$(OBJ_DIR)/%.o)

CORE_TEST_NAMES := \
	test_board \
	test_piece \
	test_move \
	test_movegen \
	test_move_execution \
	test_undo \
	test_game_state \
	test_turn \
	test_endgame \
	test_log \
	test_clock \
	test_command \
	test_input \
	test_timer \
	test_error

SYSTEM_TEST_NAMES := \
	test_event \
	test_fsm \
	test_controller \
	test_control_flow

CLI_TEST_NAMES := \
	test_cli_menu \
	test_cli_renderer \
	test_cli_gameplay \
	test_cli_app

AI_TEST_NAMES := \
	test_ai

# Placeholder or incomplete suites intentionally excluded:
# - test_rules
NON_AI_TEST_NAMES := \
	$(CORE_TEST_NAMES) \
	$(SYSTEM_TEST_NAMES) \
	$(CLI_TEST_NAMES)

TEST_NAMES := \
	$(NON_AI_TEST_NAMES) \
	$(AI_TEST_NAMES)

NON_AI_TEST_OBJS := $(NON_AI_TEST_NAMES:%=$(OBJ_DIR)/tests/%.o)
AI_TEST_OBJS := $(AI_TEST_NAMES:%=$(OBJ_DIR)/tests/%.o)
TEST_OBJS := $(NON_AI_TEST_OBJS) $(AI_TEST_OBJS)

NON_AI_TEST_BINS := $(NON_AI_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXE))
AI_TEST_BINS := $(AI_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXE))
TEST_BINS := $(NON_AI_TEST_BINS) $(AI_TEST_BINS)

DEP_FILES := \
	$(COMMON_OBJS:.o=.d) \
	$(AI_OBJS:.o=.d) \
	$(CLI_MAIN_OBJ:.o=.d) \
	$(TEST_OBJS:.o=.d)

.PHONY: all help cli tests test test-core test-system test-cli test-ai list-tests \
	rebuild clean clean-deps maybe-clean-deps run-cli print-vars

all: tests cli maybe-clean-deps

help:
	@echo "Targets:"
	@echo "  all         Build tests and the CLI executable"
	@echo "  cli         Build the CLI executable"
	@echo "  tests       Build all maintained test binaries"
	@echo "  test        Build and run all maintained tests"
	@echo "  test-core   Build and run core/service tests"
	@echo "  test-system Build and run system integration tests"
	@echo "  test-cli    Build and run CLI tests"
	@echo "  test-ai     Build and run AI tests"
	@echo "  run-cli     Build and launch the CLI executable"
	@echo "  list-tests  Print all maintained test target names"
	@echo "  clean       Remove generated build and binary artifacts"
	@echo "  clean-deps  Remove generated .d dependency files only"
	@echo "  rebuild     Clean and rebuild everything"
	@echo "Variables:"
	@echo "  KEEP_DEPS=0  Build normally, then delete generated .d files"

cli: $(CLI_APP_BIN) maybe-clean-deps

tests: $(TEST_BINS) maybe-clean-deps

test: $(TEST_BINS)
	@for test_bin in $(TEST_BINS); do "$${test_bin}"; done

test-core: $(CORE_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXE))
	@for test_bin in $(CORE_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXE)); do "$${test_bin}"; done

test-system: $(SYSTEM_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXE))
	@for test_bin in $(SYSTEM_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXE)); do "$${test_bin}"; done

test-cli: $(CLI_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXE))
	@for test_bin in $(CLI_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXE)); do "$${test_bin}"; done

test-ai: $(AI_TEST_BINS)
	@for test_bin in $(AI_TEST_BINS); do "$${test_bin}"; done

list-tests:
	@for test_name in $(TEST_NAMES); do echo "$${test_name}"; done

rebuild: clean all

clean:
	$(RMDIR) $(BUILD_DIR) $(BIN_DIR)

clean-deps:
	$(RM) $(DEP_FILES)

maybe-clean-deps:
ifeq ($(KEEP_DEPS),0)
	$(RM) $(DEP_FILES)
else
	@:
endif

run-cli: $(CLI_APP_BIN)
	./$(CLI_APP_BIN)

print-vars:
	@echo "CC=$(CC)"
	@echo "CPPFLAGS=$(CPPFLAGS)"
	@echo "CFLAGS=$(CFLAGS)"
	@echo "KEEP_DEPS=$(KEEP_DEPS)"
	@echo "CLI_APP_BIN=$(CLI_APP_BIN)"
	@echo "NON_AI_TEST_NAMES=$(NON_AI_TEST_NAMES)"
	@echo "AI_TEST_NAMES=$(AI_TEST_NAMES)"

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(TEST_BIN_DIR)/%$(EXE): $(OBJ_DIR)/tests/%.o $(COMMON_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(TEST_BIN_DIR)/test_ai$(EXE): $(OBJ_DIR)/tests/test_ai.o $(COMMON_OBJS) $(AI_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(CLI_APP_BIN): $(CLI_MAIN_OBJ) $(COMMON_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

-include $(DEP_FILES)
