#include "input/command.h"

#include <stddef.h>

/*
 * Legacy CLI alignment notes:
 * - Treat command.h as the contract for Command compatibility only.
 * - New GUI-facing input should use MoveRequest instead of Command.
 * - Input code constructs Command values, not gameplay Move objects.
 */

/* Reset a command to the explicit invalid sentinel state. */
static void markInvalidCommand(Command *cmd) {
    if (cmd == NULL) {
        return;
    }

    cmd->type = CMD_INVALID;
    cmd->from = createPosition(-1, -1);
    cmd->to = createPosition(-1, -1);
}

/* Check whether a command type is one of the non-move simple commands. */
static int isSimpleCommandType(CommandType type) {
    return type == CMD_UNDO
        || type == CMD_EXIT
        || type == CMD_BACK
        || type == CMD_NEW_GAME;
}

/* Build a move command from two validated board positions. */
int createMoveCommand(Command *cmd, Position from, Position to) {
    if (cmd == NULL || !isValidPosition(from) || !isValidPosition(to)) {
        markInvalidCommand(cmd);
        return 1;
    }

    cmd->type = CMD_MOVE;
    cmd->from = from;
    cmd->to = to;
    return 0;
}

/* Build one of the supported simple command variants. */
int createSimpleCommand(Command *cmd, CommandType type) {
    if (cmd == NULL || !isSimpleCommandType(type)) {
        markInvalidCommand(cmd);
        return 1;
    }

    cmd->type = type;
    cmd->from = createPosition(-1, -1);
    cmd->to = createPosition(-1, -1);
    return 0;
}
