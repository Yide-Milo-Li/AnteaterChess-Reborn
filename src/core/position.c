#include "core/position.h"

#include <ctype.h> /* Character type checking */
#include <stddef.h>

#include "core/board.h"

Position createPosition(int row, int col) {
    Position pos;

    pos.row = row;
    pos.col = col;
    return pos;
}

int isValidPosition(Position pos) {
    return pos.row >= 0 && pos.row < ROWS && pos.col >= 0 && pos.col < COLS;
}

Position parsePosition(const char *input) {
    const unsigned char *cursor;
    char file;
    char rank;

    /* No input */
    if (input == NULL) {
        return createPosition(-1, -1);
    }

    /* Dismiss whitespaces in the front */
    cursor = (const unsigned char *) input;
    while (*cursor != '\0' && isspace(*cursor)) {
        ++cursor;
    }

    /* Expect algebraic-style input such as A1 or j8, ignoring outer spaces. */
    if (!isalpha(*cursor)) {
        return createPosition(-1, -1);
    }

    file = (char) toupper(*cursor++);
    if (file < 'A' || file > 'J') {
        return createPosition(-1, -1);
    }

    if (*cursor < '1' || *cursor > '8') {
        return createPosition(-1, -1);
    }

    rank = (char) *cursor++;

    while (*cursor != '\0' && isspace(*cursor)) {
        ++cursor;
    }

    /* Reject trailing characters so partially valid inputs do not slip through. */
    if (*cursor != '\0') {
        return createPosition(-1, -1);
    }

    /* Board rows grow downward in the array, so rank 8 maps to row 0. */
    return createPosition(ROWS - (rank - '0'), file - 'A');
}

int positionEqual(Position a, Position b) {
    return a.row == b.row && a.col == b.col;
}
