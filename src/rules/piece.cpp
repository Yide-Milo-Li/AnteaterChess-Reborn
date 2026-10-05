#include "anteater/rules.hpp"

namespace ac {

static int isValidPieceType(PieceType type) {
    return type >= PieceType::Ant && type <= PieceType::Empty;
}

static int isValidColorValue(Color color) {
    return color >= Color::White && color <= Color::Empty;
}

Piece create_piece(PieceType type, Color color) {
    Piece piece;

    piece.type = type;
    piece.color = color;
    return piece;
}

int is_same_color(Piece a, Piece b) {
    if (!is_valid_piece(a) || !is_valid_piece(b)) {
        return 0;
    }

    return a.color == b.color;
}

char get_piece_symbol(Piece piece) {
    if (!is_valid_piece(piece) || piece.type == PieceType::Empty) {
        return '.';
    }

    switch (piece.type) {
    case PieceType::Ant:
        return 'P';
    case PieceType::Rook:
        return 'R';
    case PieceType::Knight:
        return 'N';
    case PieceType::Bishop:
        return 'B';
    case PieceType::Queen:
        return 'Q';
    case PieceType::King:
        return 'K';
    case PieceType::Anteater:
        return 'A';
    case PieceType::Empty:
    default:
        return '.';
    }
}

int is_valid_piece(Piece piece) {
    if (!isValidPieceType(piece.type) || !isValidColorValue(piece.color)) {
        return 0;
    }

    if (piece.type == PieceType::Empty) {
        return piece.color == Color::Empty;
    }

    return piece.color == Color::White || piece.color == Color::Black;
}

} // namespace ac
