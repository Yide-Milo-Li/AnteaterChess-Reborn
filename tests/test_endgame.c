#include <assert.h>
#include <stddef.h>

#include "core/gamestate.h"
#include "gameplay/endgame.h"

void clearBoardForEndgameTest(Board *board) {
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            setPiece(board, createPosition(row, col), createPiece(EMPTY_PIECE, EMPTY_COLOR));
        }
    }
}

void test_is_in_check_detects_attacks_and_blockers(void) {
    GameState state;

    initGameState(&state, NULL);
    clearBoardForEndgameTest(&state.board);

    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(7, 0), createPiece(ROOK, BLACK));

    assert(isInCheck(&state, WHITE) == 1);

    setPiece(&state.board, createPosition(7, 3), createPiece(BISHOP, WHITE));
    assert(isInCheck(&state, WHITE) == 0);
}

void test_checkmate_detection_finds_forced_mate(void) {
    GameState state;

    initGameState(&state, NULL);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = BLACK;

    setPiece(&state.board, createPosition(0, 0), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(1, 1), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(2, 2), createPiece(KING, WHITE));

    assert(isInCheck(&state, BLACK) == 1);
    assert(isCheckmate(&state, BLACK) == 1);
    assert(isStalemate(&state, BLACK) == 0);
}

void test_stalemate_detection_finds_no_legal_move_position(void) {
    GameState state;

    initGameState(&state, NULL);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = BLACK;

    setPiece(&state.board, createPosition(0, 0), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(1, 2), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(2, 2), createPiece(KING, WHITE));

    assert(isInCheck(&state, BLACK) == 0);
    assert(isStalemate(&state, BLACK) == 1);
    assert(isCheckmate(&state, BLACK) == 0);
}

void test_insufficient_material_detects_simple_draws(void) {
    GameState state;

    initGameState(&state, NULL);
    clearBoardForEndgameTest(&state.board);

    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    assert(isInsufficientMaterial(&state) == 1);

    setPiece(&state.board, createPosition(4, 4), createPiece(BISHOP, WHITE));
    assert(isInsufficientMaterial(&state) == 1);

    setPiece(&state.board, createPosition(4, 6), createPiece(QUEEN, WHITE));
    assert(isInsufficientMaterial(&state) == 0);
}

void test_detect_game_result_updates_game_state(void) {
    GameState state;

    initGameState(&state, NULL);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = BLACK;

    setPiece(&state.board, createPosition(0, 0), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(1, 1), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(2, 2), createPiece(KING, WHITE));

    assert(detectGameResult(&state) == 1);
    assert(getGameResult(&state) == RESULT_WHITE_WIN);
    assert(isGameOver(&state) == 1);

    clearBoardForEndgameTest(&state.board);
    state.currentTurn = BLACK;
    setPiece(&state.board, createPosition(0, 0), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(1, 2), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(2, 2), createPiece(KING, WHITE));

    assert(detectGameResult(&state) == 1);
    assert(getGameResult(&state) == RESULT_DRAW);
    assert(isGameOver(&state) == 1);

    initGameState(&state, NULL);
    assert(detectGameResult(&state) == 0);
    assert(getGameResult(&state) == RESULT_NONE);
    assert(isGameOver(&state) == 0);
}

int main(void) {
    test_is_in_check_detects_attacks_and_blockers();
    test_checkmate_detection_finds_forced_mate();
    test_stalemate_detection_finds_no_legal_move_position();
    test_insufficient_material_detects_simple_draws();
    test_detect_game_result_updates_game_state();
    return 0;
}
