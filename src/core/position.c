#include "core/position.h"

#include <ctype.h>
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

    if (input == NULL) {
        return createPosition(-1, -1);
    }

    cursor = (const unsigned char *) input;
    while (*cursor != '\0' && isspace(*cursor)) {
        ++cursor;
    }

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

    if (*cursor != '\0') {
        return createPosition(-1, -1);
    }

    return createPosition(ROWS - (rank - '0'), file - 'A');
}

int positionEqual(Position a, Position b) {
    return a.row == b.row && a.col == b.col;
}
