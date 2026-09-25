#include "anteater/rules.h"
#include <assert.h>
#include <stddef.h>

static void assertCaptureRecord(AcCaptureRecord record, AcSquare pos, AcPiece piece) {
    assert(ac_position_equal(record.pos, pos) == 1);
    assert(record.piece.type == piece.type);
    assert(record.piece.color == piece.color);
}

static void test_create_move_defaults(void) {
    AcSquare from = ac_create_position(6, 0);
    AcSquare to = ac_create_position(5, 0);
    AcPiece pawn = ac_create_piece(AC_ANT, AC_WHITE);
    AcMove move = ac_create_move(from, to, pawn);

    assert(ac_position_equal(move.from, from) == 1);
    assert(ac_position_equal(move.to, to) == 1);
    assert(move.movedPiece.type == AC_ANT);
    assert(move.movedPiece.color == AC_WHITE);
    assert(move.pathLength == 0);
    assert(move.captureCount == 0);
    assert(move.specialType == AC_NO_SPECIAL_MOVE);
}

static void test_move_path_and_captures(void) {
    AcMove move =
        ac_create_move(ac_create_position(2, 2), ac_create_position(4, 4), ac_create_piece(AC_ANTEATER, AC_WHITE));
    AcSquare pathPos = ac_create_position(3, 3);
    AcSquare capturePos = ac_create_position(4, 3);
    AcPiece capturePiece = ac_create_piece(AC_ANT, AC_BLACK);

    ac_add_path_step(&move, pathPos);
    ac_add_capture(&move, capturePos, capturePiece);
    ac_set_special_move(&move, AC_ANTEATER_CAPTURE);

    assert(move.pathLength == 1);
    assert(ac_position_equal(move.path[0], pathPos) == 1);
    assert(move.captureCount == 1);
    assertCaptureRecord(move.captures[0], capturePos, capturePiece);
    assert(move.specialType == AC_ANTEATER_CAPTURE);
}

static void test_move_capacity_limits(void) {
    AcMove move =
        ac_create_move(ac_create_position(0, 0), ac_create_position(1, 1), ac_create_piece(AC_QUEEN, AC_WHITE));
    int i;

    for (i = 0; i < AC_MAX_CHAIN + 2; ++i) {
        ac_add_path_step(&move, ac_create_position(i, i));
        ac_add_capture(&move, ac_create_position(i, i + 1), ac_create_piece(AC_ANT, AC_BLACK));
    }

    assert(move.pathLength == AC_MAX_CHAIN);
    assert(move.captureCount == AC_MAX_CHAIN);
    assert(ac_position_equal(move.path[AC_MAX_CHAIN - 1], ac_create_position(AC_MAX_CHAIN - 1, AC_MAX_CHAIN - 1)) == 1);
    assertCaptureRecord(move.captures[AC_MAX_CHAIN - 1], ac_create_position(AC_MAX_CHAIN - 1, AC_MAX_CHAIN),
                        ac_create_piece(AC_ANT, AC_BLACK));
}

static void test_movelist_operations(void) {
    AcMoveList list;
    AcMove move = ac_create_move(ac_create_position(6, 1), ac_create_position(5, 1), ac_create_piece(AC_ANT, AC_WHITE));
    AcMove *stored;

    ac_init_move_list(&list);
    assert(ac_get_move_count(&list) == 0);
    assert(ac_add_move(&list, move) == 0);
    assert(ac_get_move_count(&list) == 1);

    stored = ac_get_move(&list, 0);
    assert(stored != NULL);
    assert(ac_position_equal(stored->from, ac_create_position(6, 1)) == 1);
    assert(ac_get_move(&list, 1) == NULL);

    assert(ac_remove_last_move(&list) == 0);
    assert(ac_get_move_count(&list) == 0);
    assert(ac_remove_last_move(&list) != 0);
}

static void test_promotion_special_move_detection(void) {
    assert(ac_is_promotion_special_move(AC_PROMOTION_QUEEN) == 1);
    assert(ac_is_promotion_special_move(AC_PROMOTION_ROOK) == 1);
    assert(ac_is_promotion_special_move(AC_PROMOTION_BISHOP) == 1);
    assert(ac_is_promotion_special_move(AC_PROMOTION_KNIGHT) == 1);

    assert(ac_is_promotion_special_move(AC_NO_SPECIAL_MOVE) == 0);
    assert(ac_is_promotion_special_move(AC_CASTLING_KINGSIDE) == 0);
    assert(ac_is_promotion_special_move(AC_CASTLING_QUEENSIDE) == 0);
    assert(ac_is_promotion_special_move(AC_EN_PASSANT) == 0);
    assert(ac_is_promotion_special_move(AC_ANTEATER_CAPTURE) == 0);
}

int main(void) {
    test_create_move_defaults();
    test_move_path_and_captures();
    test_move_capacity_limits();
    test_movelist_operations();
    test_promotion_special_move_detection();
    return 0;
}
