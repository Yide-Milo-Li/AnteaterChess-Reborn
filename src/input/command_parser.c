#include "input/command_parser.h"

#include <stddef.h>

#include "core/position.h"

/*
 * Legacy command alignment notes:
 * - Treat command_parser.h as the contract for Command compatibility only.
 * - New GUI-facing input should use move_request_parser.h instead.
 * - This parser is a thin coordinate-to-Command adapter and must not take on
 *   event wrapping, rule validation, or move-generation behavior.
 * - Compatibility is limited to per-field case-insensitivity and leading or
 *   trailing whitespace, via parsePosition().
 */

/* Force parser failures to surface as the shared invalid command sentinel. */
static void markInvalidCommand(Command *cmd) {
    if (cmd == NULL) {
        return;
    }

    cmd->type = CMD_INVALID;
    cmd->from = createPosition(-1, -1);
    cmd->to = createPosition(-1, -1);
}

/* Parse two independent move-field strings into a move command. */
int parseMoveCommand(const char *fromText, const char *toText, Command *cmd) {
    Position from;
    Position to;

    if (cmd == NULL || fromText == NULL || toText == NULL) {
        markInvalidCommand(cmd);
        return 1;
    }

    from = parsePosition(fromText);
    to = parsePosition(toText);
    if (!isValidPosition(from) || !isValidPosition(to)) {
        markInvalidCommand(cmd);
        return 1;
    }

    return createMoveCommand(cmd, from, to);
}
