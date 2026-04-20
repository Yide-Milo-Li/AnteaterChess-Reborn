#include <assert.h>

#include "core/board.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/piece.h"
#include "input/command.h"
#include "system/event.h"
#include "system/fsm.h"
#include "system/system_state.h"

/* Build a fresh gameplay-ready state through the public FSM setup path. */
static GameState fresh_gameplay_state(void) {
    GameConfig config;
    GameState state;

    initDefaultGameConfig(&config);
    initGameState(&state, &config);
    assert(processEvent(&state, createSystemEvent(EVENT_NONE)) == 0);
    assert(processEvent(&state, createSystemEvent(EVENT_NEW_GAME)) == 0);
    assert(processEvent(&state, createSystemEvent(EVENT_NEW_GAME)) == 0);
    assert(processEvent(&state, createSystemEvent(EVENT_NEW_GAME)) == 0);
    assert(state.systemState == GAMEPLAY_STATE);
    return state;
}

/* Prepare a minimal board where one white ant has exactly one simple forward move. */
static void seed_simple_ant_position(GameState *state) {
    assert(state != NULL);
    initBoard(&state->board);
    setPiece(&state->board, createPosition(6, 0), createPiece(ANT, WHITE));
    state->currentTurn = WHITE;
    initMoveList(&state->moveHistory);
    state->moveCount = 0;
    state->gameOver = 0;
    state->result = RESULT_NONE;
}

/* Check that a valid move command is applied through the public FSM path. */
static void test_process_event_applies_valid_move(void) {
    GameState state = fresh_gameplay_state();
    Command command;

    seed_simple_ant_position(&state);
    assert(createMoveCommand(&command, createPosition(6, 0), createPosition(5, 0)) == 0);
    assert(processEvent(&state, createMoveInputEvent(command)) == 0);
    assert(getPiece(&state.board, createPosition(6, 0)).type == EMPTY_PIECE);
    assert(getPiece(&state.board, createPosition(5, 0)).type == ANT);
    assert(state.currentTurn == BLACK);
    assert(state.moveHistory.count == 1);
}

/* Check that an illegal move command is rejected without mutating the board. */
static void test_process_event_rejects_illegal_move(void) {
    GameState state = fresh_gameplay_state();
    Command command;

    seed_simple_ant_position(&state);
    assert(createMoveCommand(&command, createPosition(6, 0), createPosition(6, 1)) == 0);
    assert(processEvent(&state, createMoveInputEvent(command)) != 0);
    assert(getPiece(&state.board, createPosition(6, 0)).type == ANT);
    assert(getPiece(&state.board, createPosition(4, 0)).type == EMPTY_PIECE);
    assert(state.currentTurn == WHITE);
    assert(state.moveHistory.count == 0);
}

/* Check that undo flows back through the FSM and restores the previous position. */
static void test_process_event_undo_restores_position(void) {
    GameState state = fresh_gameplay_state();
    Command command;

    seed_simple_ant_position(&state);
    assert(createMoveCommand(&command, createPosition(6, 0), createPosition(5, 0)) == 0);
    assert(processEvent(&state, createMoveInputEvent(command)) == 0);
    assert(processEvent(&state, createUndoEvent()) == 0);
    assert(getPiece(&state.board, createPosition(6, 0)).type == ANT);
    assert(getPiece(&state.board, createPosition(5, 0)).type == EMPTY_PIECE);
    assert(state.currentTurn == WHITE);
    assert(state.moveHistory.count == 0);
}

/* Run the Phase E control-flow integration tests. */
int main(void) {
    test_process_event_applies_valid_move();
    test_process_event_rejects_illegal_move();
    test_process_event_undo_restores_position();
    return 0;
}
