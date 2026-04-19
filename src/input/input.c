#include "input/input.h"

#include <stdio.h>
#include <string.h>

#include "core/position.h"
#include "input/command_parser.h"

/*
 * Interface alignment notes for Phase F:
 * - Treat input.h as the only public contract for this file.
 * - This file is the CLI/test adapter layer, not the canonical GUI move path.
 * - GUI move fields should call parseMoveCommand(fromText, toText, &cmd)
 *   directly instead of routing through whole-line token splitting here.
 * - This layer reads raw user input and emits Command-or-primitive input data;
 *   it must not construct gameplay Move objects or wrap events directly.
 * - Any fuzzy compatibility for move fields belongs in parsePosition(), not in
 *   ad-hoc trimming or casing logic in this file.
 */

#define INPUT_BUFFER_SIZE 128
#define INPUT_TOKEN_SIZE 32

/* Reset a command to the explicit invalid sentinel state. */
static void markInvalidCommand(Command *cmd) {
    if (cmd == NULL) {
        return;
    }

    cmd->type = CMD_INVALID;
    cmd->from = createPosition(-1, -1);
    cmd->to = createPosition(-1, -1);
}

/* Read one line from stdin into a reusable fixed-size buffer. */
static int readInputLine(char *buffer, size_t size) {
    if (buffer == NULL || size == 0 || fgets(buffer, (int) size, stdin) == NULL) {
        return 0;
    }

    return 1;
}

/* Extract exactly two CLI move tokens before forwarding them to the parser. */
static int extractCliMoveTokens(const char *line, char fromToken[INPUT_TOKEN_SIZE], char toToken[INPUT_TOKEN_SIZE]) {
    char extraToken[INPUT_TOKEN_SIZE];

    if (line == NULL) {
        return 0;
    }

    return sscanf(
        line,
        " %31s %31s %31s",
        fromToken,
        toToken,
        extraToken
    ) == 2;
}

/* Parse exactly one coordinate token for board-driven input. */
static int parseBoardToken(const char *line, char token[INPUT_TOKEN_SIZE]) {
    char extraToken[INPUT_TOKEN_SIZE];

    if (line == NULL) {
        return 0;
    }

    return sscanf(line, " %31s %31s", token, extraToken) == 1;
}

/* Read one CLI move command and forward the two tokens to the shared parser. */
int getMoveInput(Command *cmd) {
    char line[INPUT_BUFFER_SIZE];
    char fromToken[INPUT_TOKEN_SIZE];
    char toToken[INPUT_TOKEN_SIZE];

    if (cmd == NULL) {
        return 1;
    }

    if (!readInputLine(line, sizeof(line)) || !extractCliMoveTokens(line, fromToken, toToken)) {
        markInvalidCommand(cmd);
        return 1;
    }

    return parseMoveCommand(fromToken, toToken, cmd);
}

/* Read one raw board coordinate without assigning GUI field semantics. */
int getBoardInput(Position *pos, InputType *type) {
    char line[INPUT_BUFFER_SIZE];
    char token[INPUT_TOKEN_SIZE];
    Position parsed;

    if (pos == NULL || type == NULL) {
        return 1;
    }

    if (!readInputLine(line, sizeof(line)) || !parseBoardToken(line, token)) {
        *pos = createPosition(-1, -1);
        *type = INPUT_CANCEL;
        return 1;
    }

    parsed = parsePosition(token);
    if (!isValidPosition(parsed)) {
        *pos = createPosition(-1, -1);
        *type = INPUT_CANCEL;
        return 1;
    }

    *pos = parsed;
    *type = INPUT_SELECT_PIECE;
    return 0;
}

/* Read one raw menu selection integer without attaching menu semantics. */
int getMenuSelection(int *selection) {
    char line[INPUT_BUFFER_SIZE];
    char extra;

    if (selection == NULL) {
        return 1;
    }

    if (!readInputLine(line, sizeof(line)) || sscanf(line, " %d %c", selection, &extra) != 1) {
        return 1;
    }

    return 0;
}
