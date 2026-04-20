CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Iinclude

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
TEST_BIN_DIR := bin/tests
BIN_DIR := bin
EXE := .exe

SRC_SRCS := \
	src/core/position.c \
	src/core/piece.c \
	src/core/board.c \
	src/core/player.c \
	src/core/move.c \
	src/core/movelist.c \
	src/core/gameconfig.c \
	src/core/gamestate.c \
	src/gameplay/execution.c \
	src/gameplay/endgame.c \
	src/gameplay/movegen.c \
	src/gameplay/rules.c \
	src/input/command.c \
	src/input/command_parser.c \
	src/input/input.c \
	src/event/event.c \
	src/event/event_queue.c \
	src/control/controller.c \
	src/control/fsm.c \
	src/error/error.c \
	src/log/log.c \
	src/time/clock.c \
	src/turn/turn.c \
	src/turn/turn_timer.c \
	src/ai/ai.c \
	src/cli/cli_feedback.c \
	src/cli/cli_renderer.c \
	src/cli/cli_menu.c \
	src/cli/cli_gameplay.c \
	src/cli/cli_app.c

SRC_OBJS := $(SRC_SRCS:src/%.c=$(OBJ_DIR)/%.o)
CLI_MAIN_OBJ := $(OBJ_DIR)/main_cli.o
CLI_APP_BIN := $(BIN_DIR)/anteater_chess_cli$(EXE)
ROOT_CLI_APP_BIN := bin_anteater_chess_cli$(EXE)

TESTS := test_board test_piece test_move test_movegen test_move_execution test_undo test_game_state test_turn test_endgame test_log test_clock test_command test_input test_timer test_event test_fsm test_controller test_control_flow test_cli_menu test_cli_renderer test_cli_gameplay test_cli_app test_error test_ai
TEST_BINS := $(TESTS:%=$(TEST_BIN_DIR)/%$(EXE))
TEST_OBJS := $(TESTS:%=$(OBJ_DIR)/tests/%.o)

.PHONY: all test clean cli

all: $(TEST_BINS) $(CLI_APP_BIN) $(ROOT_CLI_APP_BIN)

cli: $(CLI_APP_BIN) $(ROOT_CLI_APP_BIN)

test: $(TEST_BINS)
	@for test_bin in $(TEST_BINS); do "$${test_bin}"; done

clean:
	rm -rf $(BUILD_DIR) $(TEST_BIN_DIR) $(BIN_DIR)

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BIN_DIR)/%$(EXE): $(OBJ_DIR)/tests/%.o $(SRC_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

$(CLI_MAIN_OBJ): src/main_cli.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(CLI_APP_BIN): $(CLI_MAIN_OBJ) $(SRC_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

$(ROOT_CLI_APP_BIN): $(CLI_MAIN_OBJ) $(SRC_OBJS)
	$(CC) $(CFLAGS) $^ -o $@
