#include "anteater/rules.h"

#include <stddef.h>

/* Represent "no piece" with a normal AcPiece value so board access stays uniform. */
static AcPiece emptyPiece(void) {
    return ac_create_piece(AC_EMPTY_PIECE, AC_EMPTY_COLOR);
}

/* Fill one full rank with identical pieces, used for the ant rows. */
static void initializeRow(AcBoard *board, int row, AcPieceType type, AcColor color) {
    int col;

    for (col = 0; col < AC_COLS; ++col) {
        board->cells[row][col] = ac_create_piece(type, color);
    }
}

void ac_init_board(AcBoard *board) {
    /* Column order for the 10-square back rank from left to right. */
    static const AcPieceType backRank[AC_COLS] = {AC_ROOK, AC_KNIGHT,   AC_BISHOP, AC_ANTEATER, AC_QUEEN,
                                                  AC_KING, AC_ANTEATER, AC_BISHOP, AC_KNIGHT,   AC_ROOK};
    int row;
    int col;

    if (board == NULL) {
        return;
    }

    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            board->cells[row][col] = emptyPiece();
        }
    }

    /* Row 0 is the black home rank; row 7 is the white home rank. */
    for (col = 0; col < AC_COLS; ++col) {
        board->cells[0][col] = ac_create_piece(backRank[col], AC_BLACK);
        board->cells[7][col] = ac_create_piece(backRank[col], AC_WHITE);
    }

    /* Ants start directly in front of each side's back rank. */
    initializeRow(board, 1, AC_ANT, AC_BLACK);
    initializeRow(board, 6, AC_ANT, AC_WHITE);
}

AcPiece ac_get_piece(const AcBoard *board, AcSquare pos) {
    if (board == NULL || !ac_is_valid_position(pos)) {
        return emptyPiece();
    }

    return board->cells[pos.row][pos.col];
}

void ac_set_piece(AcBoard *board, AcSquare pos, AcPiece piece) {
    if (board == NULL || !ac_is_valid_position(pos)) {
        return;
    }

    board->cells[pos.row][pos.col] = piece;
}

void ac_remove_piece(AcBoard *board, AcSquare pos) {
    if (board == NULL || !ac_is_valid_position(pos)) {
        return;
    }

    board->cells[pos.row][pos.col] = emptyPiece();
}
