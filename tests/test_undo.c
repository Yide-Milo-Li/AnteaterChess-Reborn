#include <assert.h>
#include <stddef.h>

#include "core/gamestate.h"
#include "gameplay/execution.h"

void clearBoardForUndoTest(Board *board) {
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            setPiece(board, createPosition(row, col), createPiece(EMPTY_PIECE, EMPTY_COLOR));
        }
    }
}

void test_undo_move_restores_simple_move(void) {
    GameState state;
    Move move;

    initGameState(&state, NULL);
    move = createMove(createPosition(6, 4), createPosition(5, 4), createPiece(ANT, WHITE));

    assert(applyMove(&state, move) == 0);
    assert(undoMove(&state) == 0);
    assert(getPiece(&state.board, createPosition(6, 4)).type == ANT);
    assert(getPiece(&state.board, createPosition(6, 4)).color == WHITE);
    assert(getPiece(&state.board, createPosition(5, 4)).type == EMPTY_PIECE);
    assert(state.moveHistory.count == 0);
    assert(state.moveCount == 0);
    assert(state.currentTurn == WHITE);
}

void test_undo_move_restores_chain_captures(void) {
    GameState state;
    Move move;

    initGameState(&state, NULL);
    clearBoardForUndoTest(&state.board);
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
    state.result = RESULT_DRAW;
    state.gameOver = 1;

    assert(undoMove(&state) == 0);
    assert(getPiece(&state.board, createPosition(4, 4)).type == ANTEATER);
    assert(getPiece(&state.board, createPosition(4, 4)).color == WHITE);
    assert(getPiece(&state.board, createPosition(4, 5)).type == ANT);
    assert(getPiece(&state.board, createPosition(4, 5)).color == BLACK);
    assert(getPiece(&state.board, createPosition(4, 6)).type == ANT);
    assert(getPiece(&state.board, createPosition(4, 6)).color == BLACK);
    assert(state.currentTurn == WHITE);
    assert(state.result == RESULT_NONE);
    assert(state.gameOver == 0);
}

void test_undo_move_reverts_promotion_to_original_piece(void) {
    GameState state;
    Move move;

    initGameState(&state, NULL);
    clearBoardForUndoTest(&state.board);
    state.currentTurn = WHITE;

    setPiece(&state.board, createPosition(1, 2), createPiece(ANT, WHITE));
    move = createMove(createPosition(1, 2), createPosition(0, 2), createPiece(ANT, WHITE));
    setSpecialMove(&move, PROMOTION_QUEEN);

    assert(applyMove(&state, move) == 0);
    assert(undoMove(&state) == 0);
    assert(getPiece(&state.board, createPosition(1, 2)).type == ANT);
    assert(getPiece(&state.board, createPosition(1, 2)).color == WHITE);
    assert(getPiece(&state.board, createPosition(0, 2)).type == EMPTY_PIECE);
}

int main(void) {
    test_undo_move_restores_simple_move();
    test_undo_move_restores_chain_captures();
    test_undo_move_reverts_promotion_to_original_piece();
    return 0;
}
