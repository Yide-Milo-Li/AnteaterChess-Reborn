#include "input/command_parser.h"

#include <stddef.h>

#include "core/position.h"

/*
 * Interface alignment notes for Phase F:
 * - Treat command_parser.h as the only public contract for this file.
 * - This parser is the canonical GUI path for two independent move fields.
 * - It is a thin coordinate-to-Command adapter and must not take on event
 *   wrapping, rule validation, or move-generation behavior.
 * - Compatibility is limited to per-field case-insensitivity and leading or
 *   trailing whitespace, via parsePosition().
 * - Do not concatenate the two GUI fields into a single line and re-split it.
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
