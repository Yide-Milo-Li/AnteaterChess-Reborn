#include <assert.h>

#include "core/board.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/move.h"
#include "gameplay/validation.h"

/* Clear the whole board so each test can build one focused validation case. */
static void clear_board(Board *board) {
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            setPiece(board, createPosition(row, col),
                     createPiece(EMPTY_PIECE, EMPTY_COLOR));
        }
    }
}

/* Build a minimal game state for standalone rule-checking tests. */
static GameState create_test_state(Color turn) {
    GameConfig config;
    GameState state;

    initDefaultGameConfig(&config);
    initGameState(&state, &config);
    clear_board(&state.board);
    state.currentTurn = turn;
    return state;
}

/* Verify selection results differentiate empty, opponent, and invalid squares. */
static void test_validate_selection_reports_expected_result_codes(void) {
    GameState state = create_test_state(WHITE);

    setPiece(&state.board, createPosition(6, 4), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(1, 4), createPiece(ROOK, BLACK));

    assert(validateSelection(&state, createPosition(-1, 0)) == SELECT_OUT_OF_BOUNDS);
    assert(validateSelection(&state, createPosition(4, 4)) == SELECT_EMPTY);
    assert(validateSelection(&state, createPosition(6, 4)) == SELECT_VALID);
    assert(validateSelection(&state, createPosition(1, 4)) == SELECT_OPPONENT_PIECE);
}

/* Verify the rule checker accepts one ordinary legal move request. */
static void test_validate_move_accepts_basic_ant_advance(void) {
    GameState state = create_test_state(WHITE);
    Move move;

    setPiece(&state.board, createPosition(6, 4), createPiece(ANT, WHITE));

    move = createMove(createPosition(6, 4), createPosition(5, 4),
                      createPiece(ANT, WHITE));
    assert(validateMove(&state, move) == 1);
}

/* Verify off-board targets and non-playable origins are rejected cleanly. */
static void test_validate_move_rejects_invalid_origin_and_target_inputs(void) {
    GameState state = create_test_state(WHITE);
    Move emptySquareMove;
    Move opponentPieceMove;
    Move outOfBoundsMove;

    setPiece(&state.board, createPosition(6, 4), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(1, 4), createPiece(ANT, BLACK));

    emptySquareMove = createMove(createPosition(4, 4), createPosition(3, 4),
                                 createPiece(ANT, WHITE));
    opponentPieceMove = createMove(createPosition(1, 4), createPosition(2, 4),
                                   createPiece(ANT, BLACK));
    outOfBoundsMove = createMove(createPosition(6, 4), createPosition(6, 10),
                                 createPiece(ANT, WHITE));

    assert(validateMove(&state, emptySquareMove) == 0);
    assert(validateMove(&state, opponentPieceMove) == 0);
    assert(validateMove(&state, outOfBoundsMove) == 0);
}

/* Verify blocked sliding moves are not treated as legal chess moves. */
static void test_validate_move_rejects_blocked_rook_path(void) {
    GameState state = create_test_state(WHITE);
    Move blockedMove;

    setPiece(&state.board, createPosition(4, 4), createPiece(ROOK, WHITE));
    setPiece(&state.board, createPosition(4, 6), createPiece(ANT, WHITE));

    blockedMove = createMove(createPosition(4, 4), createPosition(4, 7),
                             createPiece(ROOK, WHITE));
    assert(validateMove(&state, blockedMove) == 0);
}

/* Verify explicit capture metadata must match the generated legal move exactly. */
static void test_validate_move_checks_explicit_capture_metadata(void) {
    GameState state = create_test_state(WHITE);
    Move partialRequest;
    Move exactCaptureRequest;
    Move wrongCaptureRequest;

    setPiece(&state.board, createPosition(6, 4), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(5, 5), createPiece(KNIGHT, BLACK));

    partialRequest = createMove(createPosition(6, 4), createPosition(5, 5),
                                createPiece(ANT, WHITE));

    exactCaptureRequest = createMove(createPosition(6, 4), createPosition(5, 5),
                                     createPiece(ANT, WHITE));
    addCapture(&exactCaptureRequest, createPosition(5, 5),
               createPiece(KNIGHT, BLACK));

    wrongCaptureRequest = createMove(createPosition(6, 4), createPosition(5, 5),
                                     createPiece(ANT, WHITE));
    addCapture(&wrongCaptureRequest, createPosition(5, 5),
               createPiece(BISHOP, BLACK));

    assert(validateMove(&state, partialRequest) == 1);
    assert(validateMove(&state, exactCaptureRequest) == 1);
    assert(validateMove(&state, wrongCaptureRequest) == 0);
}

int main(void) {
    test_validate_selection_reports_expected_result_codes();
    test_validate_move_accepts_basic_ant_advance();
    test_validate_move_rejects_invalid_origin_and_target_inputs();
    test_validate_move_rejects_blocked_rook_path();
    test_validate_move_checks_explicit_capture_metadata();
    return 0;
}
