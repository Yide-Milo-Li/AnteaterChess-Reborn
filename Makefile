CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Iinclude

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
TEST_BIN_DIR := bin/tests
EXE := .exe

CORE_SRCS := \
	src/core/position.c \
	src/core/piece.c \
	src/core/board.c \
	src/core/move.c \
	src/core/movelist.c \
	src/core/gameconfig.c

CORE_OBJS := $(CORE_SRCS:src/%.c=$(OBJ_DIR)/%.o)

TESTS := test_board test_piece test_move
TEST_BINS := $(TESTS:%=$(TEST_BIN_DIR)/%$(EXE))
TEST_OBJS := $(TESTS:%=$(OBJ_DIR)/tests/%.o)

.PHONY: all test clean

all: $(TEST_BINS)

test: $(TEST_BINS)
	@for test_bin in $(TEST_BINS); do "$$test_bin"; done

clean:
	rm -rf $(BUILD_DIR) $(TEST_BIN_DIR)

$(OBJ_DIR)/core/%.o: src/core/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BIN_DIR)/%$(EXE): $(OBJ_DIR)/tests/%.o $(CORE_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@
