#include "anteater/rules.hpp"
#include <assert.h>
#include <stddef.h>

using namespace ac;

static void assertCaptureRecord(CaptureRecord record, Square pos, Piece piece) {
    assert(position_equal(record.pos, pos) == 1);
    assert(record.piece.type == piece.type);
    assert(record.piece.color == piece.color);
}

static void test_create_move_defaults(void) {
    Square from = create_position(6, 0);
    Square to = create_position(5, 0);
    Piece pawn = create_piece(PieceType::Ant, Color::White);
    Move move = create_move(from, to, pawn);

    assert(position_equal(move.from, from) == 1);
    assert(position_equal(move.to, to) == 1);
    assert(move.movedPiece.type == PieceType::Ant);
    assert(move.movedPiece.color == Color::White);
    assert(move.pathLength == 0);
    assert(move.captureCount == 0);
    assert(move.specialType == SpecialMove::None);
}

static void test_move_path_and_captures(void) {
    Move move =
        create_move(create_position(2, 2), create_position(4, 4), create_piece(PieceType::Anteater, Color::White));
    Square pathPos = create_position(3, 3);
    Square capturePos = create_position(4, 3);
    Piece capturePiece = create_piece(PieceType::Ant, Color::Black);

    add_path_step(&move, pathPos);
    add_capture(&move, capturePos, capturePiece);
    set_special_move(&move, SpecialMove::AnteaterCapture);

    assert(move.pathLength == 1);
    assert(position_equal(move.path[0], pathPos) == 1);
    assert(move.captureCount == 1);
    assertCaptureRecord(move.captures[0], capturePos, capturePiece);
    assert(move.specialType == SpecialMove::AnteaterCapture);
}

static void test_move_capacity_limits(void) {
    Move move = create_move(create_position(0, 0), create_position(1, 1), create_piece(PieceType::Queen, Color::White));
    int i;

    for (i = 0; i < MaxChain + 2; ++i) {
        add_path_step(&move, create_position(i, i));
        add_capture(&move, create_position(i, i + 1), create_piece(PieceType::Ant, Color::Black));
    }

    assert(move.pathLength == MaxChain);
    assert(move.captureCount == MaxChain);
    assert(position_equal(move.path[MaxChain - 1], create_position(MaxChain - 1, MaxChain - 1)) == 1);
    assertCaptureRecord(move.captures[MaxChain - 1], create_position(MaxChain - 1, MaxChain),
                        create_piece(PieceType::Ant, Color::Black));
}

static void test_movelist_operations(void) {
    MoveList list;
    Move move = create_move(create_position(6, 1), create_position(5, 1), create_piece(PieceType::Ant, Color::White));
    Move *stored;

    init_move_list(&list);
    assert(get_move_count(&list) == 0);
    assert(add_move(&list, move) == Status::Ok);
    assert(get_move_count(&list) == 1);

    stored = get_move(&list, 0);
    assert(stored != NULL);
    assert(position_equal(stored->from, create_position(6, 1)) == 1);
    assert(get_move(&list, 1) == NULL);

    assert(remove_last_move(&list) == Status::Ok);
    assert(get_move_count(&list) == 0);
    assert(remove_last_move(&list) != Status::Ok);
}

static void test_promotion_special_move_detection(void) {
    assert(is_promotion_special_move(SpecialMove::PromotionQueen) == 1);
    assert(is_promotion_special_move(SpecialMove::PromotionRook) == 1);
    assert(is_promotion_special_move(SpecialMove::PromotionBishop) == 1);
    assert(is_promotion_special_move(SpecialMove::PromotionKnight) == 1);

    assert(is_promotion_special_move(SpecialMove::None) == 0);
    assert(is_promotion_special_move(SpecialMove::CastlingKingside) == 0);
    assert(is_promotion_special_move(SpecialMove::CastlingQueenside) == 0);
    assert(is_promotion_special_move(SpecialMove::EnPassant) == 0);
    assert(is_promotion_special_move(SpecialMove::AnteaterCapture) == 0);
}

int main(void) {
    test_create_move_defaults();
    test_move_path_and_captures();
    test_move_capacity_limits();
    test_movelist_operations();
    test_promotion_special_move_detection();
    return 0;
}
