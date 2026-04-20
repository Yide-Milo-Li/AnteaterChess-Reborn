#include <assert.h>
#include <stddef.h>

#include "core/gamestate.h"
#include "gameplay/execution.h"

void clearBoardForExecutionTest(Board *board) {
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            setPiece(board, createPosition(row, col), createPiece(EMPTY_PIECE, EMPTY_COLOR));
        }
    }
}

void test_apply_move_updates_board_history_and_turn(void) {
    GameState state;
    Move move;

    initGameState(&state, NULL);
    move = createMove(createPosition(6, 4), createPosition(5, 4), createPiece(ANT, WHITE));

    assert(applyMove(&state, move) == 0);
    assert(getPiece(&state.board, createPosition(6, 4)).type == EMPTY_PIECE);
    assert(getPiece(&state.board, createPosition(5, 4)).type == ANT);
    assert(getPiece(&state.board, createPosition(5, 4)).color == WHITE);
    assert(state.moveHistory.count == 1);
    assert(state.moveCount == 1);
    assert(state.currentTurn == BLACK);
}

void test_apply_move_removes_all_recorded_captures(void) {
    GameState state;
    Move move;

    initGameState(&state, NULL);
    clearBoardForExecutionTest(&state.board);
    state.currentTurn = WHITE;

    setPiece(&state.board, createPosition(4, 4), createPiece(ANTEATER, WHITE));
    setPiece(&state.board, createPosition(4, 5), createPiece(ANT, BLACK));
    setPiece(&state.board, createPosition(4, 6), createPiece(ANT, BLACK));

    move = createMove(createPosition(4, 4), createPosition(4, 6), createPiece(ANTEATER, WHITE));
    addPathStep(&move, createPosition(4, 5));
    addPathStep(&move, createPosition(4, 6));
    addCapture(&move, createPosition(4, 5), createPiece(ANT, BLACK));
    addCapture(&move, createPosition(4, 6), createPiece(ANT, BLACK));
    setSpecialMove(&move, ANTEATER_CAPTURE);

    assert(applyMove(&state, move) == 0);
    assert(getPiece(&state.board, createPosition(4, 4)).type == EMPTY_PIECE);
    assert(getPiece(&state.board, createPosition(4, 5)).type == EMPTY_PIECE);
    assert(getPiece(&state.board, createPosition(4, 6)).type == ANTEATER);
    assert(getPiece(&state.board, createPosition(4, 6)).color == WHITE);
}

void test_apply_move_handles_promotion(void) {
    GameState state;
    Move move;

    initGameState(&state, NULL);
    clearBoardForExecutionTest(&state.board);
    state.currentTurn = WHITE;

    setPiece(&state.board, createPosition(1, 2), createPiece(ANT, WHITE));
    move = createMove(createPosition(1, 2), createPosition(0, 2), createPiece(ANT, WHITE));
    setSpecialMove(&move, PROMOTION_QUEEN);

    assert(applyMove(&state, move) == 0);
    assert(getPiece(&state.board, createPosition(0, 2)).type == QUEEN);
    assert(getPiece(&state.board, createPosition(0, 2)).color == WHITE);
}

void test_apply_move_handles_castling(void) {
    GameState state;
    Move move;

    initGameState(&state, NULL);
    clearBoardForExecutionTest(&state.board);
    state.currentTurn = WHITE;

    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(7, 9), createPiece(ROOK, WHITE));

    move = createMove(createPosition(7, 5), createPosition(7, 7), createPiece(KING, WHITE));
    setSpecialMove(&move, CASTLING_KINGSIDE);

    assert(applyMove(&state, move) == 0);
    assert(getPiece(&state.board, createPosition(7, 5)).type == EMPTY_PIECE);
    assert(getPiece(&state.board, createPosition(7, 7)).type == KING);
    assert(getPiece(&state.board, createPosition(7, 6)).type == ROOK);
    assert(getPiece(&state.board, createPosition(7, 9)).type == EMPTY_PIECE);
}

void test_apply_move_handles_en_passant(void) {
    GameState state;
    Move move;

    initGameState(&state, NULL);
    clearBoardForExecutionTest(&state.board);
    state.currentTurn = WHITE;

    setPiece(&state.board, createPosition(3, 4), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(3, 5), createPiece(ANT, BLACK));

    move = createMove(createPosition(3, 4), createPosition(2, 5), createPiece(ANT, WHITE));
    addCapture(&move, createPosition(3, 5), createPiece(ANT, BLACK));
    setSpecialMove(&move, EN_PASSANT);

    assert(applyMove(&state, move) == 0);
    assert(getPiece(&state.board, createPosition(3, 4)).type == EMPTY_PIECE);
    assert(getPiece(&state.board, createPosition(3, 5)).type == EMPTY_PIECE);
    assert(getPiece(&state.board, createPosition(2, 5)).type == ANT);
    assert(getPiece(&state.board, createPosition(2, 5)).color == WHITE);
}

int main(void) {
    test_apply_move_updates_board_history_and_turn();
    test_apply_move_removes_all_recorded_captures();
    test_apply_move_handles_promotion();
    test_apply_move_handles_castling();
    test_apply_move_handles_en_passant();
    return 0;
}
