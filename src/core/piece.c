#include "core/piece.h"

static int isValidPieceType(PieceType type) {
    return type >= ANT && type <= EMPTY_PIECE;
}

static int isValidColorValue(Color color) {
    return color >= WHITE && color <= EMPTY_COLOR;
}

Piece createPiece(PieceType type, Color color) {
    Piece piece;

    piece.type = type;
    piece.color = color;
    return piece;
}

int isSameColor(Piece a, Piece b) {
    if (!isValidPiece(a) || !isValidPiece(b)) {
        return 0;
    }

    return a.color == b.color;
}

char getPieceSymbol(Piece piece) {
    if (!isValidPiece(piece) || piece.type == EMPTY_PIECE) {
        return '.';
    }

    switch (piece.type) {
        case ANT:
            return 'P';
        case ROOK:
            return 'R';
        case KNIGHT:
            return 'N';
        case BISHOP:
            return 'B';
        case QUEEN:
            return 'Q';
        case KING:
            return 'K';
        case ANTEATER:
            return 'A';
        case EMPTY_PIECE:
        default:
            return '.';
    }
}

int isValidPiece(Piece piece) {
    if (!isValidPieceType(piece.type) || !isValidColorValue(piece.color)) {
        return 0;
    }

    if (piece.type == EMPTY_PIECE) {
        return piece.color == EMPTY_COLOR;
    }

    return piece.color == WHITE || piece.color == BLACK;
}
