#include "anteater/rules.h"
#include <assert.h>

static void test_create_piece(void) {
    AcPiece piece = ac_create_piece(AC_BISHOP, AC_WHITE);

    assert(piece.type == AC_BISHOP);
    assert(piece.color == AC_WHITE);
}

static void test_same_color_checks(void) {
    assert(ac_is_same_color(ac_create_piece(AC_ROOK, AC_WHITE), ac_create_piece(AC_KING, AC_WHITE)) == 1);
    assert(ac_is_same_color(ac_create_piece(AC_ROOK, AC_WHITE), ac_create_piece(AC_KING, AC_BLACK)) == 0);
    assert(ac_is_same_color(ac_create_piece(AC_EMPTY_PIECE, AC_EMPTY_COLOR),
                            ac_create_piece(AC_EMPTY_PIECE, AC_EMPTY_COLOR)) == 1);
}

static void test_piece_symbols(void) {
    assert(ac_get_piece_symbol(ac_create_piece(AC_ANT, AC_WHITE)) == 'P');
    assert(ac_get_piece_symbol(ac_create_piece(AC_ROOK, AC_BLACK)) == 'R');
    assert(ac_get_piece_symbol(ac_create_piece(AC_KNIGHT, AC_WHITE)) == 'N');
    assert(ac_get_piece_symbol(ac_create_piece(AC_BISHOP, AC_WHITE)) == 'B');
    assert(ac_get_piece_symbol(ac_create_piece(AC_QUEEN, AC_BLACK)) == 'Q');
    assert(ac_get_piece_symbol(ac_create_piece(AC_KING, AC_WHITE)) == 'K');
    assert(ac_get_piece_symbol(ac_create_piece(AC_ANTEATER, AC_BLACK)) == 'A');
    assert(ac_get_piece_symbol(ac_create_piece(AC_EMPTY_PIECE, AC_EMPTY_COLOR)) == '.');
}

static void test_piece_validity(void) {
    assert(ac_is_valid_piece(ac_create_piece(AC_ANT, AC_WHITE)) == 1);
    assert(ac_is_valid_piece(ac_create_piece(AC_EMPTY_PIECE, AC_EMPTY_COLOR)) == 1);
    assert(ac_is_valid_piece(ac_create_piece(AC_QUEEN, AC_EMPTY_COLOR)) == 0);
    assert(ac_is_valid_piece(ac_create_piece(AC_EMPTY_PIECE, AC_WHITE)) == 0);
    assert(ac_is_valid_piece(ac_create_piece((AcPieceType)-1, AC_WHITE)) == 0);
    assert(ac_is_valid_piece(ac_create_piece(AC_ROOK, (AcColor)99)) == 0);
}

int main(void) {
    test_create_piece();
    test_same_color_checks();
    test_piece_symbols();
    test_piece_validity();
    return 0;
}
