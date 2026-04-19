CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Iinclude

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
TEST_BIN_DIR := bin/tests
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
	src/log/log.c \
	src/time/clock.c \
	src/turn/turn.c

SRC_OBJS := $(SRC_SRCS:src/%.c=$(OBJ_DIR)/%.o)

TESTS := test_board test_piece test_move test_movegen test_move_execution test_undo test_game_state test_turn test_endgame test_log test_clock
TEST_BINS := $(TESTS:%=$(TEST_BIN_DIR)/%$(EXE))
TEST_OBJS := $(TESTS:%=$(OBJ_DIR)/tests/%.o)

.PHONY: all test clean

all: $(TEST_BINS)

test: $(TEST_BINS)
	@for test_bin in $(TEST_BINS); do "$$test_bin"; done

clean:
	rm -rf $(BUILD_DIR) $(TEST_BIN_DIR)

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BIN_DIR)/%$(EXE): $(OBJ_DIR)/tests/%.o $(SRC_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@
