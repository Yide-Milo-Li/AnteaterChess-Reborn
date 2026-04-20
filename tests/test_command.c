#include <assert.h>
#include <stddef.h>

#include "core/position.h"
#include "input/command.h"
#include "input/command_parser.h"

/* Check that move commands are created only from valid board coordinates. */
static void test_create_move_command(void) {
    Command cmd;

    assert(createMoveCommand(&cmd, createPosition(6, 4), createPosition(4, 4)) == 0);
    assert(cmd.type == CMD_MOVE);
    assert(positionEqual(cmd.from, createPosition(6, 4)) == 1);
    assert(positionEqual(cmd.to, createPosition(4, 4)) == 1);

    assert(createMoveCommand(&cmd, createPosition(-1, 4), createPosition(4, 4)) != 0);
    assert(cmd.type == CMD_INVALID);
    assert(createMoveCommand(NULL, createPosition(6, 4), createPosition(4, 4)) != 0);
}

/* Check that simple commands accept only the non-move command variants. */
static void test_create_simple_command(void) {
    Command cmd;

    assert(createSimpleCommand(&cmd, CMD_UNDO) == 0);
    assert(cmd.type == CMD_UNDO);
    assert(createSimpleCommand(&cmd, CMD_EXIT) == 0);
    assert(cmd.type == CMD_EXIT);
    assert(createSimpleCommand(&cmd, CMD_BACK) == 0);
    assert(cmd.type == CMD_BACK);
    assert(createSimpleCommand(&cmd, CMD_NEW_GAME) == 0);
    assert(cmd.type == CMD_NEW_GAME);
    assert(createSimpleCommand(&cmd, CMD_MOVE) != 0);
    assert(cmd.type == CMD_INVALID);
    assert(createSimpleCommand(&cmd, CMD_INVALID) != 0);
}

/* Check that coordinate parsing accepts GUI field whitespace but rejects malformed input. */
static void test_parse_move_command(void) {
    Command cmd;

    assert(parseMoveCommand("E2", "E4", &cmd) == 0);
    assert(cmd.type == CMD_MOVE);
    assert(positionEqual(cmd.from, createPosition(6, 4)) == 1);
    assert(positionEqual(cmd.to, createPosition(4, 4)) == 1);

    assert(parseMoveCommand("e2", "e4", &cmd) == 0);
    assert(parseMoveCommand("  e2", "E4  ", &cmd) == 0);
    assert(parseMoveCommand("  e2  ", "  e4  ", &cmd) == 0);
    assert(parseMoveCommand("K2", "E4", &cmd) != 0);
    assert(cmd.type == CMD_INVALID);
    assert(parseMoveCommand("", "E4", &cmd) != 0);
    assert(parseMoveCommand(NULL, "E4", &cmd) != 0);
    assert(parseMoveCommand("E 2", "E4", &cmd) != 0);
    assert(parseMoveCommand("E2", "E 4", &cmd) != 0);
    assert(parseMoveCommand("E2 E4", "", &cmd) != 0);
    assert(parseMoveCommand("E2x", "E4", &cmd) != 0);
}

/* Run the Phase F command tests. */
int main(void) {
    test_create_move_command();
    test_create_simple_command();
    test_parse_move_command();
    return 0;
}
