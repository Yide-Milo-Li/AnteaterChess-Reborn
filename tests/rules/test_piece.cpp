#include "anteater/rules.hpp"
#include <assert.h>

using namespace ac;

static void test_create_piece(void) {
    Piece piece = create_piece(PieceType::Bishop, Color::White);

    assert(piece.type == PieceType::Bishop);
    assert(piece.color == Color::White);
}

static void test_same_color_checks(void) {
    assert(is_same_color(create_piece(PieceType::Rook, Color::White), create_piece(PieceType::King, Color::White)) ==
           1);
    assert(is_same_color(create_piece(PieceType::Rook, Color::White), create_piece(PieceType::King, Color::Black)) ==
           0);
    assert(is_same_color(create_piece(PieceType::Empty, Color::Empty), create_piece(PieceType::Empty, Color::Empty)) ==
           1);
}

static void test_piece_symbols(void) {
    assert(get_piece_symbol(create_piece(PieceType::Ant, Color::White)) == 'P');
    assert(get_piece_symbol(create_piece(PieceType::Rook, Color::Black)) == 'R');
    assert(get_piece_symbol(create_piece(PieceType::Knight, Color::White)) == 'N');
    assert(get_piece_symbol(create_piece(PieceType::Bishop, Color::White)) == 'B');
    assert(get_piece_symbol(create_piece(PieceType::Queen, Color::Black)) == 'Q');
    assert(get_piece_symbol(create_piece(PieceType::King, Color::White)) == 'K');
    assert(get_piece_symbol(create_piece(PieceType::Anteater, Color::Black)) == 'A');
    assert(get_piece_symbol(create_piece(PieceType::Empty, Color::Empty)) == '.');
}

static void test_piece_validity(void) {
    assert(is_valid_piece(create_piece(PieceType::Ant, Color::White)) == 1);
    assert(is_valid_piece(create_piece(PieceType::Empty, Color::Empty)) == 1);
    assert(is_valid_piece(create_piece(PieceType::Queen, Color::Empty)) == 0);
    assert(is_valid_piece(create_piece(PieceType::Empty, Color::White)) == 0);
    assert(is_valid_piece(create_piece((PieceType)-1, Color::White)) == 0);
    assert(is_valid_piece(create_piece(PieceType::Rook, (Color)99)) == 0);
}

int main(void) {
    test_create_piece();
    test_same_color_checks();
    test_piece_symbols();
    test_piece_validity();
    return 0;
}
