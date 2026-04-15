#include "core/board.h"

#include <stddef.h>

/* Represent "no piece" with a normal Piece value so board access stays uniform. */
static Piece emptyPiece(void) {
    return createPiece(EMPTY_PIECE, EMPTY_COLOR);
}

/* Fill one full rank with identical pieces, used for the ant rows. */
static void initializeRow(Board *board, int row, PieceType type, Color color) {
    int col;

    for (col = 0; col < COLS; ++col) {
        board->cells[row][col] = createPiece(type, color);
    }
}

void initBoard(Board *board) {
    /* Column order for the 10-square back rank from left to right. */
    static const PieceType backRank[COLS] = {
        ROOK, KNIGHT, BISHOP, ANTEATER, QUEEN,
        KING, ANTEATER, BISHOP, KNIGHT, ROOK
    };
    int row;
    int col;

    if (board == NULL) {
        return;
    }

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            board->cells[row][col] = emptyPiece();
        }
    }

    /* Row 0 is the black home rank; row 7 is the white home rank. */
    for (col = 0; col < COLS; ++col) {
        board->cells[0][col] = createPiece(backRank[col], BLACK);
        board->cells[7][col] = createPiece(backRank[col], WHITE);
    }

    /* Ants start directly in front of each side's back rank. */
    initializeRow(board, 1, ANT, BLACK);
    initializeRow(board, 6, ANT, WHITE);
}

Piece getPiece(const Board *board, Position pos) {
    if (board == NULL || !isValidPosition(pos)) {
        return emptyPiece();
    }

    return board->cells[pos.row][pos.col];
}

void setPiece(Board *board, Position pos, Piece piece) {
    if (board == NULL || !isValidPosition(pos)) {
        return;
    }

    board->cells[pos.row][pos.col] = piece;
}

void removePiece(Board *board, Position pos) {
    if (board == NULL || !isValidPosition(pos)) {
        return;
    }

    board->cells[pos.row][pos.col] = emptyPiece();
}
