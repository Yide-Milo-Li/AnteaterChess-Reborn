.DEFAULT_GOAL := all

CC ?= gcc
CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Werror
LDFLAGS ?=
LDLIBS ?=
DEPFLAGS := -MMD -MP

RM ?= rm -f
RMDIR ?= rm -rf
MKDIR_P ?= mkdir -p
CP ?= cp -R
TAR ?= tar

EXEEXT ?=
ifeq ($(OS),Windows_NT)
EXEEXT := .exe
endif

BIN_DIR := bin
LOG_DIR := $(BIN_DIR)/logs
TEST_BIN_DIR := $(BIN_DIR)/tests
CHESS_BIN := $(BIN_DIR)/chess$(EXEEXT)

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
PKG_DIR := $(BUILD_DIR)/package
SRC_STAGE_DIR := $(PKG_DIR)/Chess_Alpha_src
SRC_ARCHIVE := Chess_Alpha_src.tar.gz

README_SRC := $(firstword $(wildcard README README_DEV))
INSTALL_SRC := $(firstword $(wildcard INSTALL INSTALL_DEV))

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

APP_SRCS := \
	$(CORE_SRCS) \
	$(GAMEPLAY_SRCS) \
	$(INPUT_SRCS) \
	$(SYSTEM_SRCS) \
	$(SERVICE_SRCS) \
	$(CLI_SRCS) \
	$(AI_SRCS)

APP_OBJS := $(APP_SRCS:src/%.c=$(OBJ_DIR)/%.o)
CLI_MAIN_OBJ := $(CLI_MAIN_SRC:src/%.c=$(OBJ_DIR)/%.o)

CORE_TEST_NAMES := \
	test_board \
	test_piece \
	test_move \
	test_rulecheck \
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
	test_boarddisplay \
	test_cli_menu \
	test_cli_renderer \
	test_cli_gameplay \
	test_cli_app

AI_TEST_NAMES := \
	test_ai

TEST_NAMES := \
	$(CORE_TEST_NAMES) \
	$(SYSTEM_TEST_NAMES) \
	$(CLI_TEST_NAMES) \
	$(AI_TEST_NAMES)

TEST_OBJS := $(TEST_NAMES:%=$(OBJ_DIR)/tests/%.o)
TEST_BINS := $(TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT))

DEP_FILES := \
	$(APP_OBJS:.o=.d) \
	$(CLI_MAIN_OBJ:.o=.d) \
	$(TEST_OBJS:.o=.d)

.PHONY: all cli tests test test-core test-system test-cli test-ai \
	list-tests run clean tar help

all: $(CHESS_BIN) $(LOG_DIR)

cli: all

tests: $(TEST_BINS)

test: tests
	@set -e; for test_bin in $(TEST_BINS); do "./$$test_bin"; done

test-core: $(CORE_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT))
	@set -e; for test_bin in $(CORE_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT)); do "./$$test_bin"; done

test-system: $(SYSTEM_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT))
	@set -e; for test_bin in $(SYSTEM_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT)); do "./$$test_bin"; done

test-cli: $(CLI_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT))
	@set -e; for test_bin in $(CLI_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT)); do "./$$test_bin"; done

test-ai: $(AI_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT))
	@set -e; for test_bin in $(AI_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT)); do "./$$test_bin"; done

list-tests:
	@printf '%s\n' $(TEST_NAMES)

run: $(CHESS_BIN)
	./$(CHESS_BIN)

clean:
	$(RMDIR) $(BUILD_DIR)
	$(RM) $(SRC_ARCHIVE) $(CHESS_BIN) $(TEST_BINS) $(wildcard *.o) $(wildcard *.d)
	$(RM) $(wildcard $(LOG_DIR)/*) $(wildcard $(TEST_BIN_DIR)/*)
	$(MKDIR_P) $(BIN_DIR) $(LOG_DIR) $(TEST_BIN_DIR)

tar: $(SRC_ARCHIVE)

help:
	@echo "Targets:"
	@echo "  make / make all   Build bin/chess and create bin/logs"
	@echo "  make test         Build and run the maintained test suite"
	@echo "  make clean        Remove generated binaries, objects, logs, and tarball while preserving bin/"
	@echo "  make tar          Create Chess_Alpha_src.tar.gz"
	@echo "  make list-tests   Print the maintained test binary names"

$(SRC_ARCHIVE): Makefile $(README_SRC) $(INSTALL_SRC) COPYRIGHT
	$(RMDIR) $(PKG_DIR)
	$(MKDIR_P) $(SRC_STAGE_DIR)/bin $(SRC_STAGE_DIR)/bin/logs $(SRC_STAGE_DIR)/doc
	cp $(README_SRC) $(SRC_STAGE_DIR)/README
	cp $(INSTALL_SRC) $(SRC_STAGE_DIR)/INSTALL
	cp COPYRIGHT $(SRC_STAGE_DIR)/
	cp Makefile $(SRC_STAGE_DIR)/
	$(CP) include $(SRC_STAGE_DIR)/
	$(CP) src $(SRC_STAGE_DIR)/
	$(CP) tests $(SRC_STAGE_DIR)/
	@if [ -d doc ]; then $(CP) doc/. $(SRC_STAGE_DIR)/doc/; fi
	$(TAR) -czf $(SRC_ARCHIVE) -C $(PKG_DIR) Chess_Alpha_src

$(LOG_DIR):
	$(MKDIR_P) $@

$(BIN_DIR) $(TEST_BIN_DIR):
	$(MKDIR_P) $@

$(OBJ_DIR)/%.o: src/%.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(CHESS_BIN): $(BIN_DIR) $(LOG_DIR) $(CLI_MAIN_OBJ) $(APP_OBJS)
	$(CC) $(LDFLAGS) $(CLI_MAIN_OBJ) $(APP_OBJS) $(LDLIBS) -o $@

$(TEST_BIN_DIR)/%$(EXEEXT): $(TEST_BIN_DIR) $(OBJ_DIR)/tests/%.o $(APP_OBJS)
	$(CC) $(LDFLAGS) $(OBJ_DIR)/tests/$*.o $(APP_OBJS) $(LDLIBS) -o $@

-include $(DEP_FILES)
