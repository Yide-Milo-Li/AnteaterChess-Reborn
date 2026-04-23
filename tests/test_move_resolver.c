#include <assert.h>

#include "core/board.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/movelist.h"
#include "core/piece.h"
#include "gameplay/move_resolver.h"
#include "input/move_request.h"

static void clear_board(GameState *state) {
    int row;
    int col;

    assert(state != NULL);
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            setPiece(&state->board, createPosition(row, col),
                createPiece(EMPTY_PIECE, EMPTY_COLOR));
        }
    }
}

static GameState fresh_empty_state(Color turn) {
    GameConfig config;
    GameState state;

    initDefaultGameConfig(&config);
    initGameState(&state, &config);
    clear_board(&state);
    state.currentTurn = turn;
    initMoveList(&state.moveHistory);
    state.moveCount = 0;
    state.gameOver = 0;
    state.result = RESULT_NONE;
    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    return state;
}

static void push_history_move(GameState *state, Move move) {
    assert(state != NULL);
    assert(addMove(&state->moveHistory, move) == 0);
    ++state->moveCount;
}

static void test_resolve_simple_move(void) {
    GameState state = fresh_empty_state(WHITE);
    MoveRequest request;
    Move move;

    setPiece(&state.board, createPosition(6, 0), createPiece(ANT, WHITE));
    assert(createMoveRequest(&request, createPosition(6, 0),
        createPosition(5, 0), PROMOTION_CHOICE_NONE) == 0);

    assert(resolveMoveRequest(&state, request, &move) == 0);
    assert(positionEqual(move.from, createPosition(6, 0)));
    assert(positionEqual(move.to, createPosition(5, 0)));
    assert(move.specialType == NO_SPECIAL_MOVE);
}

static void test_resolve_castling(void) {
    GameState state = fresh_empty_state(WHITE);
    MoveRequest request;
    Move move;

    setPiece(&state.board, createPosition(7, 9), createPiece(ROOK, WHITE));
    assert(createMoveRequest(&request, createPosition(7, 5),
        createPosition(7, 7), PROMOTION_CHOICE_NONE) == 0);

    assert(resolveMoveRequest(&state, request, &move) == 0);
    assert(move.specialType == CASTLING_KINGSIDE);
}

static void test_resolve_en_passant(void) {
    GameState state = fresh_empty_state(WHITE);
    MoveRequest request;
    Move move;

    setPiece(&state.board, createPosition(3, 4), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(3, 5), createPiece(ANT, BLACK));
    push_history_move(&state,
        createMove(createPosition(1, 5), createPosition(3, 5), createPiece(ANT, BLACK)));

    assert(createMoveRequest(&request, createPosition(3, 4),
        createPosition(2, 5), PROMOTION_CHOICE_NONE) == 0);

    assert(resolveMoveRequest(&state, request, &move) == 0);
    assert(move.specialType == EN_PASSANT);
    assert(move.captureCount == 1);
    assert(positionEqual(move.captures[0].pos, createPosition(3, 5)));
}

static void test_resolve_promotion_defaults_to_queen(void) {
    GameState state = fresh_empty_state(WHITE);
    MoveRequest request;
    Move move;

    setPiece(&state.board, createPosition(1, 2), createPiece(ANT, WHITE));
    assert(createMoveRequest(&request, createPosition(1, 2),
        createPosition(0, 2), PROMOTION_CHOICE_NONE) == 0);

    assert(resolveMoveRequest(&state, request, &move) == 0);
    assert(move.specialType == PROMOTION_QUEEN);
}

static void test_resolve_explicit_promotion_choices(void) {
    GameState state = fresh_empty_state(WHITE);
    MoveRequest request;
    Move move;

    setPiece(&state.board, createPosition(1, 2), createPiece(ANT, WHITE));

    assert(createMoveRequest(&request, createPosition(1, 2),
        createPosition(0, 2), PROMOTION_CHOICE_ROOK) == 0);
    assert(resolveMoveRequest(&state, request, &move) == 0);
    assert(move.specialType == PROMOTION_ROOK);

    request.promotion = PROMOTION_CHOICE_BISHOP;
    assert(resolveMoveRequest(&state, request, &move) == 0);
    assert(move.specialType == PROMOTION_BISHOP);

    request.promotion = PROMOTION_CHOICE_KNIGHT;
    assert(resolveMoveRequest(&state, request, &move) == 0);
    assert(move.specialType == PROMOTION_KNIGHT);
}

static void test_resolve_rejects_invalid_request(void) {
    GameState state = fresh_empty_state(WHITE);
    MoveRequest request;
    Move move;

    setPiece(&state.board, createPosition(1, 2), createPiece(ANT, WHITE));
    assert(createMoveRequest(&request, createPosition(1, 2),
        createPosition(0, 2), PROMOTION_CHOICE_QUEEN) == 0);
    request.promotion = (PromotionChoice)99;

    assert(resolveMoveRequest(&state, request, &move) != 0);
}

int main(void) {
    test_resolve_simple_move();
    test_resolve_castling();
    test_resolve_en_passant();
    test_resolve_promotion_defaults_to_queen();
    test_resolve_explicit_promotion_choices();
    test_resolve_rejects_invalid_request();
    return 0;
}
