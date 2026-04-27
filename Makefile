.DEFAULT_GOAL := all

CC ?= gcc
CPPFLAGS ?= -Iinclude -Isrc -D_POSIX_C_SOURCE=200809L
CFLAGS ?= -std=c11 -Wall -Wextra -Werror
LDFLAGS ?=
LDLIBS ?=
GTK_CFLAGS ?= $(shell pkg-config --cflags gtk+-3.0 2>/dev/null)
GTK_LIBS ?= $(shell pkg-config --libs gtk+-3.0 2>/dev/null)
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
SRC_STAGE_DIR := $(PKG_DIR)/Chess_V1.0_src
SRC_ARCHIVE := Chess_V1.0_src.tar.gz
USER_STAGE_DIR := $(PKG_DIR)/Chess_V1.0
USER_ARCHIVE := Chess_V1.0.tar.gz

README_SRC := $(firstword $(wildcard packaging/src/README README))
INSTALL_SRC := $(firstword $(wildcard packaging/src/INSTALL INSTALL))
USER_README_SRC := $(firstword $(wildcard packaging/user/README))
USER_INSTALL_SRC := $(firstword $(wildcard packaging/user/INSTALL))
USER_MANUAL_PDF := doc/Chess_UserManual.pdf
ASSET_FILES := $(wildcard assets/*)

CORE_SRCS := \
	src/core/position.c \
	src/core/piece.c \
	src/core/board.c \
	src/core/player.c \
	src/core/move.c \
	src/core/movelist.c \
	src/core/gameconfig.c \
	src/core/gamestate.c \
	src/core/hash.c

GAMEPLAY_SRCS := \
	src/gameplay/execution.c \
	src/gameplay/endgame.c \
	src/gameplay/movegen.c \
	src/gameplay/move_resolver.c \
	src/gameplay/rules.c

INPUT_SRCS := \
	src/input/move_request.c \
	src/input/move_request_parser.c

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

AI_SRCS := \
	src/ai/ai.c

GUI_SRCS := \
	src/ui/gui.c \
	src/ui/gui_actions.c \
	src/ui/gui_async.c \
	src/ui/gui_common.c \
	src/ui/gui_format.c \
	src/ui/gui_gameplay_screen.c \
	src/ui/gui_screens.c \
	src/ui/gui_style.c \
	src/ui/gui_setup_screen.c

GUI_MAIN_SRC := src/main.c

APP_SRCS := \
	$(CORE_SRCS) \
	$(GAMEPLAY_SRCS) \
	$(INPUT_SRCS) \
	$(SYSTEM_SRCS) \
	$(SERVICE_SRCS)

GUI_APP_SRCS := \
	$(APP_SRCS) \
	$(AI_SRCS) \
	$(GUI_SRCS)

TEST_APP_SRCS := \
	$(APP_SRCS) \
	$(AI_SRCS)

GUI_APP_OBJS := $(GUI_APP_SRCS:src/%.c=$(OBJ_DIR)/%.o)
TEST_APP_OBJS := $(TEST_APP_SRCS:src/%.c=$(OBJ_DIR)/%.o)
GUI_MAIN_OBJ := $(GUI_MAIN_SRC:src/%.c=$(OBJ_DIR)/%.o)

CORE_TEST_NAMES := \
	test_board \
	test_piece \
	test_move \
	test_rulecheck \
	test_movegen \
	test_move_resolver \
	test_move_execution \
	test_game_state \
	test_endgame \
	test_log \
	test_clock \
	test_timer \
	test_error \
	test_move_request_parser

SYSTEM_TEST_NAMES := \
	test_event \
	test_fsm \
	test_controller \
	test_control_flow

AI_TEST_NAMES := \
	test_ai

TEST_NAMES := \
	$(CORE_TEST_NAMES) \
	$(SYSTEM_TEST_NAMES) \
	$(AI_TEST_NAMES)

TEST_OBJS := $(TEST_NAMES:%=$(OBJ_DIR)/tests/%.o)
TEST_BINS := $(TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT))

DEP_FILES := \
	$(GUI_APP_OBJS:.o=.d) \
	$(TEST_APP_OBJS:.o=.d) \
	$(GUI_MAIN_OBJ:.o=.d) \
	$(TEST_OBJS:.o=.d)

ARCHIVE_SRC_DEPS := \
	$(GUI_APP_SRCS) $(TEST_APP_SRCS) $(GUI_MAIN_SRC) \
	$(wildcard tests/*.c) \
	$(ASSET_FILES) \
	$(shell find include -name '*.h' 2>/dev/null)

.PHONY: all gui tests test test-core test-system test-ai \
	list-tests run clean tar tar-user help

all: gui

gui: $(CHESS_BIN) $(LOG_DIR)

tests: $(TEST_BINS)

test: tests
	@set -e; for test_bin in $(TEST_BINS); do "./$$test_bin"; done

test-core: $(CORE_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT))
	@set -e; for test_bin in $(CORE_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT)); do "./$$test_bin"; done

test-system: $(SYSTEM_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT))
	@set -e; for test_bin in $(SYSTEM_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT)); do "./$$test_bin"; done

test-ai: $(AI_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT))
	@set -e; for test_bin in $(AI_TEST_NAMES:%=$(TEST_BIN_DIR)/%$(EXEEXT)); do "./$$test_bin"; done

list-tests:
	@printf '%s\n' $(TEST_NAMES)

run: $(CHESS_BIN)
	./$(CHESS_BIN)

clean:
	$(RMDIR) $(BUILD_DIR)
	$(RM) $(SRC_ARCHIVE) $(USER_ARCHIVE) $(CHESS_BIN) $(TEST_BINS) $(wildcard *.o) $(wildcard *.d)
	$(RM) $(wildcard $(LOG_DIR)/*) $(wildcard $(TEST_BIN_DIR)/*)
	$(MKDIR_P) $(BIN_DIR) $(LOG_DIR) $(TEST_BIN_DIR)

tar: $(SRC_ARCHIVE)

tar-user: $(USER_ARCHIVE)

help:
	@echo "Targets:"
	@echo "  make / make all   Build GUI bin/chess (requires GTK+ 3) and create bin/logs"
	@echo "  make test         Build and run the full test suite"
	@echo "  make clean        Remove generated binaries, objects, logs, and tarballs while preserving bin/"
	@echo "  make tar          Create Chess_V1.0_src.tar.gz (source package)"
	@if [ -f packaging/user/README ] && [ -f packaging/user/INSTALL ]; then \
		echo "  make tar-user     Create Chess_V1.0.tar.gz (user package, requires doc/Chess_UserManual.pdf)"; \
	fi
	@echo "  make list-tests   Print the maintained test binary names"
	@echo "  make run          Build and launch the GUI (./bin/chess)"

$(SRC_ARCHIVE): Makefile $(README_SRC) $(INSTALL_SRC) COPYRIGHT $(ARCHIVE_SRC_DEPS)
	$(RMDIR) $(SRC_STAGE_DIR)
	$(MKDIR_P) $(SRC_STAGE_DIR)/bin $(SRC_STAGE_DIR)/bin/logs $(SRC_STAGE_DIR)/doc
	cp $(README_SRC) $(SRC_STAGE_DIR)/README
	cp $(INSTALL_SRC) $(SRC_STAGE_DIR)/INSTALL
	cp COPYRIGHT $(SRC_STAGE_DIR)/
	cp Makefile $(SRC_STAGE_DIR)/
	$(CP) include $(SRC_STAGE_DIR)/
	$(CP) src $(SRC_STAGE_DIR)/
	$(CP) tests $(SRC_STAGE_DIR)/
	$(CP) assets $(SRC_STAGE_DIR)/
	@if [ -d doc ]; then $(CP) doc/. $(SRC_STAGE_DIR)/doc/; fi
	$(TAR) -czf $(SRC_ARCHIVE) -C $(PKG_DIR) Chess_V1.0_src

$(USER_ARCHIVE): $(CHESS_BIN) $(USER_README_SRC) $(USER_INSTALL_SRC) COPYRIGHT $(ASSET_FILES)
	$(RMDIR) $(USER_STAGE_DIR)
	$(MKDIR_P) $(USER_STAGE_DIR)/bin $(USER_STAGE_DIR)/bin/logs $(USER_STAGE_DIR)/doc
	cp $(USER_README_SRC) $(USER_STAGE_DIR)/README
	cp $(USER_INSTALL_SRC) $(USER_STAGE_DIR)/INSTALL
	cp COPYRIGHT $(USER_STAGE_DIR)/
	cp $(CHESS_BIN) $(USER_STAGE_DIR)/bin/chess
	$(CP) assets $(USER_STAGE_DIR)/
	@if [ -f $(USER_MANUAL_PDF) ]; then \
		cp $(USER_MANUAL_PDF) $(USER_STAGE_DIR)/doc/; \
	else \
		echo "WARNING: $(USER_MANUAL_PDF) not found; user package will ship without the user manual."; \
	fi
	$(TAR) -czf $(USER_ARCHIVE) -C $(PKG_DIR) Chess_V1.0

$(LOG_DIR):
	$(MKDIR_P) $@

$(BIN_DIR) $(TEST_BIN_DIR):
	$(MKDIR_P) $@

$(OBJ_DIR)/%.o: src/%.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(GTK_CFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(CHESS_BIN): $(BIN_DIR) $(LOG_DIR) $(GUI_MAIN_OBJ) $(GUI_APP_OBJS)
	$(CC) $(LDFLAGS) $(GUI_MAIN_OBJ) $(GUI_APP_OBJS) $(LDLIBS) $(GTK_LIBS) -pthread -o $@

$(TEST_BIN_DIR)/%$(EXEEXT): $(TEST_BIN_DIR) $(OBJ_DIR)/tests/%.o $(TEST_APP_OBJS)
	$(CC) $(LDFLAGS) $(OBJ_DIR)/tests/$*.o $(TEST_APP_OBJS) $(LDLIBS) -o $@

ifneq ($(filter clean,$(MAKECMDGOALS)),clean)
-include $(DEP_FILES)
endif
