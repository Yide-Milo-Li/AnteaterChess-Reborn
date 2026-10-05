#include "anteater/rules.hpp"

#include <stddef.h>

namespace ac {

/* Represent "no piece" with a normal Piece value so board access stays uniform. */
static Piece emptyPiece(void) {
    return create_piece(PieceType::Empty, Color::Empty);
}

/* Fill one full rank with identical pieces, used for the ant rows. */
static void initializeRow(Board *board, int row, PieceType type, Color color) {
    int col;

    for (col = 0; col < Columns; ++col) {
        board->cells[row][col] = create_piece(type, color);
    }
}

void init_board(Board *board) {
    /* Column order for the 10-square back rank from left to right. */
    static const PieceType backRank[Columns] = {
        PieceType::Rook, PieceType::Knight,   PieceType::Bishop, PieceType::Anteater, PieceType::Queen,
        PieceType::King, PieceType::Anteater, PieceType::Bishop, PieceType::Knight,   PieceType::Rook};
    int row;
    int col;

    if (board == NULL) {
        return;
    }

    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            board->cells[row][col] = emptyPiece();
        }
    }

    /* Row 0 is the black home rank; row 7 is the white home rank. */
    for (col = 0; col < Columns; ++col) {
        board->cells[0][col] = create_piece(backRank[col], Color::Black);
        board->cells[7][col] = create_piece(backRank[col], Color::White);
    }

    /* Ants start directly in front of each side's back rank. */
    initializeRow(board, 1, PieceType::Ant, Color::Black);
    initializeRow(board, 6, PieceType::Ant, Color::White);
}

Piece get_piece(const Board *board, Square pos) {
    if (board == NULL || !is_valid_position(pos)) {
        return emptyPiece();
    }

    return board->cells[pos.row][pos.col];
}

void set_piece(Board *board, Square pos, Piece piece) {
    if (board == NULL || !is_valid_position(pos)) {
        return;
    }

    board->cells[pos.row][pos.col] = piece;
}

void remove_piece(Board *board, Square pos) {
    if (board == NULL || !is_valid_position(pos)) {
        return;
    }

    board->cells[pos.row][pos.col] = emptyPiece();
}

} // namespace ac
