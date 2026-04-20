#include "input/command.h"

#include <stddef.h>

/*
 * Interface alignment notes for Phase F:
 * - Treat command.h as the only public contract for this file.
 * - Input code constructs Command values, not gameplay Move objects.
 * - Text parsing is limited to two coordinate tokens in this phase.
 * - GUI input should also be normalized into Command later, but that state
 *   machine belongs above the input module.
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
