#include <assert.h>

#include "core/piece.h"

static void test_create_piece(void) {
    Piece piece = createPiece(BISHOP, WHITE);

    assert(piece.type == BISHOP);
    assert(piece.color == WHITE);
}

static void test_same_color_checks(void) {
    assert(isSameColor(createPiece(ROOK, WHITE), createPiece(KING, WHITE)) == 1);
    assert(isSameColor(createPiece(ROOK, WHITE), createPiece(KING, BLACK)) == 0);
    assert(isSameColor(createPiece(EMPTY_PIECE, EMPTY_COLOR),
        createPiece(EMPTY_PIECE, EMPTY_COLOR)) == 1);
}

static void test_piece_symbols(void) {
    assert(getPieceSymbol(createPiece(ANT, WHITE)) == 'P');
    assert(getPieceSymbol(createPiece(ROOK, BLACK)) == 'R');
    assert(getPieceSymbol(createPiece(KNIGHT, WHITE)) == 'N');
    assert(getPieceSymbol(createPiece(BISHOP, WHITE)) == 'B');
    assert(getPieceSymbol(createPiece(QUEEN, BLACK)) == 'Q');
    assert(getPieceSymbol(createPiece(KING, WHITE)) == 'K');
    assert(getPieceSymbol(createPiece(ANTEATER, BLACK)) == 'A');
    assert(getPieceSymbol(createPiece(EMPTY_PIECE, EMPTY_COLOR)) == '.');
}

static void test_piece_validity(void) {
    assert(isValidPiece(createPiece(ANT, WHITE)) == 1);
    assert(isValidPiece(createPiece(EMPTY_PIECE, EMPTY_COLOR)) == 1);
    assert(isValidPiece(createPiece(QUEEN, EMPTY_COLOR)) == 0);
    assert(isValidPiece(createPiece(EMPTY_PIECE, WHITE)) == 0);
    assert(isValidPiece(createPiece((PieceType) -1, WHITE)) == 0);
    assert(isValidPiece(createPiece(ROOK, (Color) 99)) == 0);
}

int main(void) {
    test_create_piece();
    test_same_color_checks();
    test_piece_symbols();
    test_piece_validity();
    return 0;
}
