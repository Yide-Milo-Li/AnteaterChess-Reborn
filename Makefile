# Anteater Chess build configuration
#
# Alignment assumptions for future extensions:
# - The CLI application is the only runnable frontend wired into this Makefile today.
# - GUI and AI sources remain intentionally excluded until their public contracts are stable.
# - Test targets only include suites that currently have maintained entrypoints and link cleanly.

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
ROOT_CLI_APP_BIN := bin_$(CLI_APP_NAME)$(EXE)

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

CLI_MAIN_SRC := src/main_cli.c

COMMON_SRCS := \
	$(CORE_SRCS) \
	$(GAMEPLAY_SRCS) \
	$(INPUT_SRCS) \
	$(SYSTEM_SRCS) \
	$(SERVICE_SRCS) \
	$(CLI_SRCS)

COMMON_OBJS := $(COMMON_SRCS:src/%.c=$(OBJ_DIR)/%.o)
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
	test_timer

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

TEST_NAMES := \
	$(CORE_TEST_NAMES) \
	$(SYSTEM_TEST_NAMES) \
	$(CLI_TEST_NAMES)

# Placeholder or incomplete suites intentionally excluded for now:
# - test_ai
# - test_error
# - test_rules
TEST_OBJS := $(TEST_NAMES:%=$(OBJ_DIR)/tests/%.o)
TEST_BINS := $(TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXE))

DEP_FILES := \
	$(COMMON_OBJS:.o=.d) \
	$(CLI_MAIN_OBJ:.o=.d) \
	$(TEST_OBJS:.o=.d)

.PHONY: all help apps cli tests test list-tests rebuild clean clean-deps run-cli print-vars

all: tests cli maybe-clean-deps

help:
	@echo "Targets:"
	@echo "  all        Build tests and CLI applications"
	@echo "  cli        Build CLI executables"
	@echo "  tests      Build all maintained test binaries"
	@echo "  test       Build and run all maintained test binaries"
	@echo "  run-cli    Build and launch the bin CLI executable"
	@echo "  list-tests Print the maintained test target names"
	@echo "  clean-deps Remove generated .d dependency files only"
	@echo "  clean      Remove generated build and binary artifacts"
	@echo "  rebuild    Clean and rebuild everything"
	@echo "  print-vars Print key Makefile variables for debugging"
	@echo "Variables:"
	@echo "  KEEP_DEPS=0  Build normally, then delete generated .d files"

apps: cli

cli: $(CLI_APP_BIN) $(ROOT_CLI_APP_BIN) maybe-clean-deps

tests: $(TEST_BINS) maybe-clean-deps

test: $(TEST_BINS)
	@for test_bin in $(TEST_BINS); do "$${test_bin}"; done

list-tests:
	@for test_name in $(TEST_NAMES); do echo "$${test_name}"; done

rebuild: clean all

clean:
	$(RMDIR) $(BUILD_DIR) $(BIN_DIR) $(ROOT_CLI_APP_BIN)

clean-deps:
	$(RM) $(DEP_FILES)

run-cli: $(CLI_APP_BIN)
	./$(CLI_APP_BIN)

print-vars:
	@echo "CC=$(CC)"
	@echo "CPPFLAGS=$(CPPFLAGS)"
	@echo "CFLAGS=$(CFLAGS)"
	@echo "KEEP_DEPS=$(KEEP_DEPS)"
	@echo "CLI_APP_BIN=$(CLI_APP_BIN)"
	@echo "ROOT_CLI_APP_BIN=$(ROOT_CLI_APP_BIN)"
	@echo "TEST_NAMES=$(TEST_NAMES)"

maybe-clean-deps:
ifeq ($(KEEP_DEPS),0)
	$(RM) $(DEP_FILES)
else
	@:
endif

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(TEST_BIN_DIR)/%$(EXE): $(OBJ_DIR)/tests/%.o $(COMMON_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(CLI_APP_BIN): $(CLI_MAIN_OBJ) $(COMMON_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(ROOT_CLI_APP_BIN): $(CLI_MAIN_OBJ) $(COMMON_OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

-include $(DEP_FILES)
