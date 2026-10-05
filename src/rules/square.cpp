#include "anteater/rules.h"

#include <ctype.h> /* Character type checking */
#include <stddef.h>

AcSquare ac_create_position(int row, int col) {
    AcSquare pos;

    pos.row = row;
    pos.col = col;
    return pos;
}

int ac_is_valid_position(AcSquare pos) {
    return pos.row >= 0 && pos.row < AC_ROWS && pos.col >= 0 && pos.col < AC_COLS;
}

AcSquare ac_parse_position(const char *input) {
    const unsigned char *cursor;
    char file;
    char rank;

    /* No input */
    if (input == NULL) {
        return ac_create_position(-1, -1);
    }

    /* Dismiss whitespaces in the front */
    cursor = (const unsigned char *)input;
    while (*cursor != '\0' && isspace(*cursor)) {
        ++cursor;
    }

    /* Expect algebraic-style input such as A1 or j8, ignoring outer spaces. */
    if (!isalpha(*cursor)) {
        return ac_create_position(-1, -1);
    }

    file = (char)toupper(*cursor++);
    if (file < 'A' || file > 'J') {
        return ac_create_position(-1, -1);
    }

    if (*cursor < '1' || *cursor > '8') {
        return ac_create_position(-1, -1);
    }

    rank = (char)*cursor++;

    while (*cursor != '\0' && isspace(*cursor)) {
        ++cursor;
    }

    /* Reject trailing characters so partially valid inputs do not slip through. */
    if (*cursor != '\0') {
        return ac_create_position(-1, -1);
    }

    /* AcBoard rows grow downward in the array, so rank 8 maps to row 0. */
    return ac_create_position(AC_ROWS - (rank - '0'), file - 'A');
}

int ac_position_equal(AcSquare a, AcSquare b) {
    return a.row == b.row && a.col == b.col;
}
