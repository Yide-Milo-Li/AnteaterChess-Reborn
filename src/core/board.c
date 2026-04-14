#include "core/board.h"

#include <stddef.h>

static Piece emptyPiece(void) {
    return createPiece(EMPTY_PIECE, EMPTY_COLOR);
}

static void initializeRow(Board *board, int row, PieceType type, Color color) {
    int col;

    for (col = 0; col < COLS; ++col) {
        board->cells[row][col] = createPiece(type, color);
    }
}

void initBoard(Board *board) {
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

    for (col = 0; col < COLS; ++col) {
        board->cells[0][col] = createPiece(backRank[col], BLACK);
        board->cells[7][col] = createPiece(backRank[col], WHITE);
    }

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
