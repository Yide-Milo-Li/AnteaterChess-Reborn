#include "anteater/rules.h"

static int isValidPieceType(AcPieceType type) {
    return type >= AC_ANT && type <= AC_EMPTY_PIECE;
}

static int isValidColorValue(AcColor color) {
    return color >= AC_WHITE && color <= AC_EMPTY_COLOR;
}

AcPiece ac_create_piece(AcPieceType type, AcColor color) {
    AcPiece piece;

    piece.type = type;
    piece.color = color;
    return piece;
}

int ac_is_same_color(AcPiece a, AcPiece b) {
    if (!ac_is_valid_piece(a) || !ac_is_valid_piece(b)) {
        return 0;
    }

    return a.color == b.color;
}

char ac_get_piece_symbol(AcPiece piece) {
    if (!ac_is_valid_piece(piece) || piece.type == AC_EMPTY_PIECE) {
        return '.';
    }

    switch (piece.type) {
    case AC_ANT:
        return 'P';
    case AC_ROOK:
        return 'R';
    case AC_KNIGHT:
        return 'N';
    case AC_BISHOP:
        return 'B';
    case AC_QUEEN:
        return 'Q';
    case AC_KING:
        return 'K';
    case AC_ANTEATER:
        return 'A';
    case AC_EMPTY_PIECE:
    default:
        return '.';
    }
}

int ac_is_valid_piece(AcPiece piece) {
    if (!isValidPieceType(piece.type) || !isValidColorValue(piece.color)) {
        return 0;
    }

    if (piece.type == AC_EMPTY_PIECE) {
        return piece.color == AC_EMPTY_COLOR;
    }

    return piece.color == AC_WHITE || piece.color == AC_BLACK;
}
