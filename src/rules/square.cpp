#include "anteater/rules.hpp"

#include <ctype.h> /* Character type checking */
#include <stddef.h>

namespace ac {

Square create_position(int row, int col) {
    Square pos;

    pos.row = row;
    pos.col = col;
    return pos;
}

int is_valid_position(Square pos) {
    return pos.row >= 0 && pos.row < Rows && pos.col >= 0 && pos.col < Columns;
}

Square parse_position(const char *input) {
    const unsigned char *cursor;
    char file;
    char rank;

    /* No input */
    if (input == NULL) {
        return create_position(-1, -1);
    }

    /* Dismiss whitespaces in the front */
    cursor = (const unsigned char *)input;
    while (*cursor != '\0' && isspace(*cursor)) {
        ++cursor;
    }

    /* Expect algebraic-style input such as A1 or j8, ignoring outer spaces. */
    if (!isalpha(*cursor)) {
        return create_position(-1, -1);
    }

    file = (char)toupper(*cursor++);
    if (file < 'A' || file > 'J') {
        return create_position(-1, -1);
    }

    if (*cursor < '1' || *cursor > '8') {
        return create_position(-1, -1);
    }

    rank = (char)*cursor++;

    while (*cursor != '\0' && isspace(*cursor)) {
        ++cursor;
    }

    /* Reject trailing characters so partially valid inputs do not slip through. */
    if (*cursor != '\0') {
        return create_position(-1, -1);
    }

    /* Board rows grow downward in the array, so rank 8 maps to row 0. */
    return create_position(Rows - (rank - '0'), file - 'A');
}

int position_equal(Square a, Square b) {
    return a.row == b.row && a.col == b.col;
}

} // namespace ac
