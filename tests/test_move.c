#include <assert.h>
#include <stddef.h>

#include "core/move.h"
#include "core/movelist.h"

static void assertCaptureRecord(CaptureRecord record, Position pos, Piece piece) {
    assert(positionEqual(record.pos, pos) == 1);
    assert(record.piece.type == piece.type);
    assert(record.piece.color == piece.color);
}

static void test_create_move_defaults(void) {
    Position from = createPosition(6, 0);
    Position to = createPosition(5, 0);
    Piece pawn = createPiece(ANT, WHITE);
    Move move = createMove(from, to, pawn);

    assert(positionEqual(move.from, from) == 1);
    assert(positionEqual(move.to, to) == 1);
    assert(move.movedPiece.type == ANT);
    assert(move.movedPiece.color == WHITE);
    assert(move.pathLength == 0);
    assert(move.captureCount == 0);
    assert(move.specialType == NO_SPECIAL_MOVE);
}

static void test_move_path_and_captures(void) {
    Move move = createMove(createPosition(2, 2), createPosition(4, 4), createPiece(ANTEATER, WHITE));
    Position pathPos = createPosition(3, 3);
    Position capturePos = createPosition(4, 3);
    Piece capturePiece = createPiece(ANT, BLACK);

    addPathStep(&move, pathPos);
    addCapture(&move, capturePos, capturePiece);
    setSpecialMove(&move, ANTEATER_CAPTURE);

    assert(move.pathLength == 1);
    assert(positionEqual(move.path[0], pathPos) == 1);
    assert(move.captureCount == 1);
    assertCaptureRecord(move.captures[0], capturePos, capturePiece);
    assert(move.specialType == ANTEATER_CAPTURE);
}

static void test_move_capacity_limits(void) {
    Move move = createMove(createPosition(0, 0), createPosition(1, 1), createPiece(QUEEN, WHITE));
    int i;

    for (i = 0; i < MAX_CHAIN + 2; ++i) {
        addPathStep(&move, createPosition(i, i));
        addCapture(&move, createPosition(i, i + 1), createPiece(ANT, BLACK));
    }

    assert(move.pathLength == MAX_CHAIN);
    assert(move.captureCount == MAX_CHAIN);
    assert(positionEqual(move.path[MAX_CHAIN - 1], createPosition(MAX_CHAIN - 1, MAX_CHAIN - 1)) == 1);
    assertCaptureRecord(
        move.captures[MAX_CHAIN - 1],
        createPosition(MAX_CHAIN - 1, MAX_CHAIN),
        createPiece(ANT, BLACK)
    );
}

static void test_movelist_operations(void) {
    MoveList list;
    Move move = createMove(createPosition(6, 1), createPosition(5, 1), createPiece(ANT, WHITE));
    Move *stored;

    initMoveList(&list);
    assert(getMoveCount(&list) == 0);
    assert(addMove(&list, move) == 0);
    assert(getMoveCount(&list) == 1);

    stored = getMove(&list, 0);
    assert(stored != NULL);
    assert(positionEqual(stored->from, createPosition(6, 1)) == 1);
    assert(getMove(&list, 1) == NULL);

    assert(removeLastMove(&list) == 0);
    assert(getMoveCount(&list) == 0);
    assert(removeLastMove(&list) != 0);
}

static void test_promotion_special_move_detection(void) {
    assert(isPromotionSpecialMove(PROMOTION_QUEEN) == 1);
    assert(isPromotionSpecialMove(PROMOTION_ROOK) == 1);
    assert(isPromotionSpecialMove(PROMOTION_BISHOP) == 1);
    assert(isPromotionSpecialMove(PROMOTION_KNIGHT) == 1);

    assert(isPromotionSpecialMove(NO_SPECIAL_MOVE) == 0);
    assert(isPromotionSpecialMove(CASTLING_KINGSIDE) == 0);
    assert(isPromotionSpecialMove(CASTLING_QUEENSIDE) == 0);
    assert(isPromotionSpecialMove(EN_PASSANT) == 0);
    assert(isPromotionSpecialMove(ANTEATER_CAPTURE) == 0);
}

int main(void) {
    test_create_move_defaults();
    test_move_path_and_captures();
    test_move_capacity_limits();
    test_movelist_operations();
    test_promotion_special_move_detection();
    return 0;
}
