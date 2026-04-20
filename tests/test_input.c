#include <assert.h>
#include <stdio.h>

#include "core/position.h"
#include "input/command.h"
#include "input/input.h"

#define INPUT_FIXTURE_DIR "tests/fixtures/input/"

/* Replace stdin with a test fixture file for one input-oriented assertion. */
static void writeFixtureAndRedirect(const char *path, const char *contents) {
    FILE *file = fopen(path, "w");

    assert(file != NULL);
    fputs(contents, file);
    fclose(file);
    assert(freopen(path, "r", stdin) != NULL);
}

/* Check that move input consumes exactly two valid coordinate tokens. */
static void test_get_move_input(void) {
    Command cmd;

    writeFixtureAndRedirect(INPUT_FIXTURE_DIR "phase_f_move_valid.txt", "E2 E4\n");
    assert(getMoveInput(&cmd) == 0);
    assert(cmd.type == CMD_MOVE);
    assert(positionEqual(cmd.from, createPosition(6, 4)) == 1);
    assert(positionEqual(cmd.to, createPosition(4, 4)) == 1);

    writeFixtureAndRedirect(INPUT_FIXTURE_DIR "phase_f_move_compatible.txt", "  e2   E4  \n");
    assert(getMoveInput(&cmd) == 0);
    assert(cmd.type == CMD_MOVE);
    assert(positionEqual(cmd.from, createPosition(6, 4)) == 1);
    assert(positionEqual(cmd.to, createPosition(4, 4)) == 1);

    writeFixtureAndRedirect(INPUT_FIXTURE_DIR "phase_f_move_missing.txt", "E2\n");
    assert(getMoveInput(&cmd) != 0);
    assert(cmd.type == CMD_INVALID);

    writeFixtureAndRedirect(INPUT_FIXTURE_DIR "phase_f_move_invalid.txt", "Z9 E4\n");
    assert(getMoveInput(&cmd) != 0);
    assert(cmd.type == CMD_INVALID);
}

/* Check that board input reads a single coordinate or returns cancel on error. */
static void test_get_board_input(void) {
    Position pos;
    InputType type;

    writeFixtureAndRedirect(INPUT_FIXTURE_DIR "phase_f_board_valid.txt", "B7\n");
    assert(getBoardInput(&pos, &type) == 0);
    assert(positionEqual(pos, createPosition(1, 1)) == 1);
    assert(type == INPUT_SELECT_PIECE);

    writeFixtureAndRedirect(INPUT_FIXTURE_DIR "phase_f_board_invalid.txt", "B7 C6\n");
    assert(getBoardInput(&pos, &type) != 0);
    assert(type == INPUT_CANCEL);

    writeFixtureAndRedirect(INPUT_FIXTURE_DIR "phase_f_board_bad_coord.txt", "Z1\n");
    assert(getBoardInput(&pos, &type) != 0);
    assert(type == INPUT_CANCEL);
}

/* Check that menu input accepts only a clean integer selection. */
static void test_get_menu_selection(void) {
    int selection = 0;

    writeFixtureAndRedirect(INPUT_FIXTURE_DIR "phase_f_menu_valid.txt", "2\n");
    assert(getMenuSelection(&selection) == 0);
    assert(selection == 2);

    writeFixtureAndRedirect(INPUT_FIXTURE_DIR "phase_f_menu_invalid.txt", "2 extra\n");
    assert(getMenuSelection(&selection) != 0);

    writeFixtureAndRedirect(INPUT_FIXTURE_DIR "phase_f_menu_not_number.txt", "abc\n");
    assert(getMenuSelection(&selection) != 0);
}

/* Run the Phase F input tests. */
int main(void) {
    test_get_move_input();
    test_get_board_input();
    test_get_menu_selection();
    return 0;
}
