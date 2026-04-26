#include <assert.h>
#include <stddef.h>
#include "core/board.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/move.h"
#include "core/movelist.h"
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

/* Prepare a minimal board where one white ant has exactly one simple forward
 * move. */
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

/* Rebuild a minimal gameplay position directly on the public state. */
static void seed_empty_gameplay_position(GameState *state, Color turn) {
    int row;
    int col;

    assert(state != NULL);
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            setPiece(&state->board, createPosition(row, col),
                createPiece(EMPTY_PIECE, EMPTY_COLOR));
        }
    }

    state->currentTurn = turn;
    initMoveList(&state->moveHistory);
    state->moveCount = 0;
    state->gameOver = 0;
    state->result = RESULT_NONE;
}

/* Configure one gameplay-ready state for human-vs-computer ownership. */
static void seed_human_vs_computer_players(GameState *state, Color humanColor) {
    assert(state != NULL);

    state->config.mode = MODE_HUMAN_VS_COMPUTER;
    if (humanColor == WHITE) {
        state->players[WHITE] = createPlayer(WHITE, HUMAN);
        state->players[BLACK] = createPlayer(BLACK, AI);
    } else {
        state->players[WHITE] = createPlayer(WHITE, AI);
        state->players[BLACK] = createPlayer(BLACK, HUMAN);
    }
}

/* Record one historical move without mutating the current board state. */
static void push_history_move(GameState *state, Move move) {
    assert(state != NULL);
    assert(addMove(&state->moveHistory, move) == 0);
    ++state->moveCount;
}

/* Check that a valid move command is applied through the public FSM path. */
static void test_process_event_applies_valid_move(void) {
    GameState state = fresh_gameplay_state();
    Command command;

    seed_simple_ant_position(&state);
    assert(createMoveCommand(&command, createPosition(6, 0),
                             createPosition(5, 0)) == 0);
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
    assert(createMoveCommand(&command, createPosition(6, 0),
                             createPosition(6, 1)) == 0);
    assert(processEvent(&state, createMoveInputEvent(command)) != 0);
    assert(getPiece(&state.board, createPosition(6, 0)).type == ANT);
    assert(getPiece(&state.board, createPosition(4, 0)).type == EMPTY_PIECE);
    assert(state.currentTurn == WHITE);
    assert(state.moveHistory.count == 0);
}

/* Check that undo flows back through the FSM and restores the previous
 * position. */
static void test_process_event_undo_restores_position(void) {
    GameState state = fresh_gameplay_state();
    Command command;

    seed_simple_ant_position(&state);
    assert(createMoveCommand(&command, createPosition(6, 0),
                             createPosition(5, 0)) == 0);
    assert(processEvent(&state, createMoveInputEvent(command)) == 0);
    assert(processEvent(&state, createUndoEvent()) == 0);
    assert(getPiece(&state.board, createPosition(6, 0)).type == ANT);
    assert(getPiece(&state.board, createPosition(5, 0)).type == EMPTY_PIECE);
    assert(state.currentTurn == WHITE);
    assert(state.moveHistory.count == 0);
}

/* Check that human-vs-computer undo rewinds White games to the previous human
 * turn instead of stopping on the AI turn. */
static void test_process_event_hvc_undo_returns_to_previous_white_human_turn(void) {
    GameState state = fresh_gameplay_state();
    Command command;
    Move aiMove;

    seed_empty_gameplay_position(&state, WHITE);
    seed_human_vs_computer_players(&state, WHITE);
    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(6, 0), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(1, 0), createPiece(ANT, BLACK));

    assert(createMoveCommand(&command, createPosition(6, 0),
        createPosition(5, 0)) == 0);
    assert(processEvent(&state, createMoveInputEvent(command)) == 0);

    aiMove = createMove(createPosition(1, 0), createPosition(2, 0), createPiece(ANT, BLACK));
    assert(processEvent(&state, createAIMoveEvent(aiMove)) == 0);
    assert(state.currentTurn == WHITE);
    assert(state.moveHistory.count == 2);

    assert(processEvent(&state, createUndoEvent()) == 0);
    assert(getPiece(&state.board, createPosition(6, 0)).type == ANT);
    assert(getPiece(&state.board, createPosition(5, 0)).type == EMPTY_PIECE);
    assert(getPiece(&state.board, createPosition(1, 0)).type == ANT);
    assert(getPiece(&state.board, createPosition(2, 0)).type == EMPTY_PIECE);
    assert(state.currentTurn == WHITE);
    assert(state.moveHistory.count == 0);
}

/* Check that human-vs-computer undo rewinds Black games to the previous human
 * turn after White's opening AI move. */
static void test_process_event_hvc_undo_returns_to_previous_black_human_turn(void) {
    GameState state = fresh_gameplay_state();
    Command command;
    Move aiOpeningMove;
    Move aiReplyMove;

    seed_empty_gameplay_position(&state, WHITE);
    seed_human_vs_computer_players(&state, BLACK);
    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(6, 0), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(1, 0), createPiece(ANT, BLACK));

    aiOpeningMove = createMove(createPosition(6, 0), createPosition(5, 0), createPiece(ANT, WHITE));
    assert(processEvent(&state, createAIMoveEvent(aiOpeningMove)) == 0);

    assert(createMoveCommand(&command, createPosition(1, 0),
        createPosition(2, 0)) == 0);
    assert(processEvent(&state, createMoveInputEvent(command)) == 0);

    aiReplyMove = createMove(createPosition(5, 0), createPosition(4, 0), createPiece(ANT, WHITE));
    assert(processEvent(&state, createAIMoveEvent(aiReplyMove)) == 0);
    assert(state.currentTurn == BLACK);
    assert(state.moveHistory.count == 3);

    assert(processEvent(&state, createUndoEvent()) == 0);
    assert(getPiece(&state.board, createPosition(5, 0)).type == ANT);
    assert(getPiece(&state.board, createPosition(4, 0)).type == EMPTY_PIECE);
    assert(getPiece(&state.board, createPosition(1, 0)).type == ANT);
    assert(getPiece(&state.board, createPosition(2, 0)).type == EMPTY_PIECE);
    assert(state.currentTurn == BLACK);
    assert(state.moveHistory.count == 1);
}

/* Check that Black cannot undo White's opening AI move before Black has taken
 * a turn. */
static void test_process_event_hvc_black_opening_undo_unavailable(void) {
    GameState state = fresh_gameplay_state();
    Move aiOpeningMove;

    seed_empty_gameplay_position(&state, WHITE);
    seed_human_vs_computer_players(&state, BLACK);
    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(6, 0), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(1, 0), createPiece(ANT, BLACK));

    aiOpeningMove = createMove(createPosition(6, 0), createPosition(5, 0), createPiece(ANT, WHITE));
    assert(processEvent(&state, createAIMoveEvent(aiOpeningMove)) == 0);

    assert(processEvent(&state, createUndoEvent()) != 0);
    assert(getPiece(&state.board, createPosition(5, 0)).type == ANT);
    assert(getPiece(&state.board, createPosition(6, 0)).type == EMPTY_PIECE);
    assert(getPiece(&state.board, createPosition(1, 0)).type == ANT);
    assert(state.currentTurn == BLACK);
    assert(state.moveHistory.count == 1);
}

/* Check that public move input auto-promotes to queen without changing the
 * command interface. */
static void test_process_event_auto_promotes_to_queen(void) {
    GameState state = fresh_gameplay_state();
    Command command;

    seed_empty_gameplay_position(&state, WHITE);
    setPiece(&state.board, createPosition(1, 2), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));

    assert(createMoveCommand(&command, createPosition(1, 2),
        createPosition(0, 2)) == 0);
    assert(processEvent(&state, createMoveInputEvent(command)) == 0);
    assert(getPiece(&state.board, createPosition(0, 2)).type == QUEEN);
    assert(getPiece(&state.board, createPosition(0, 2)).color == WHITE);
}

/* Check that castling resolves correctly from the existing from/to command
 * input format. */
static void test_process_event_applies_castling(void) {
    GameState state = fresh_gameplay_state();
    Command command;

    seed_empty_gameplay_position(&state, WHITE);
    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(7, 9), createPiece(ROOK, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));

    assert(createMoveCommand(&command, createPosition(7, 5),
        createPosition(7, 7)) == 0);
    assert(processEvent(&state, createMoveInputEvent(command)) == 0);
    assert(getPiece(&state.board, createPosition(7, 7)).type == KING);
    assert(getPiece(&state.board, createPosition(7, 6)).type == ROOK);
}

/* Check that en passant resolves correctly from the existing from/to command
 * input format and latest move history. */
static void test_process_event_applies_en_passant(void) {
    GameState state = fresh_gameplay_state();
    Command command;

    seed_empty_gameplay_position(&state, WHITE);
    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(3, 4), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(3, 5), createPiece(ANT, BLACK));
    push_history_move(&state,
        createMove(createPosition(1, 5), createPosition(3, 5), createPiece(ANT, BLACK)));

    assert(createMoveCommand(&command, createPosition(3, 4),
        createPosition(2, 5)) == 0);
    assert(processEvent(&state, createMoveInputEvent(command)) == 0);
    assert(getPiece(&state.board, createPosition(2, 5)).type == ANT);
    assert(getPiece(&state.board, createPosition(3, 5)).type == EMPTY_PIECE);
}

/* Check that timer expiry skips the active turn and keeps gameplay running. */
static void test_process_event_timer_expiry_passes_turn(void) {
    GameState state = fresh_gameplay_state();

    seed_empty_gameplay_position(&state, WHITE);
    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));

    assert(processEvent(&state, createSystemEvent(EVENT_TIMER_EXPIRED)) == 0);
    assert(state.systemState == GAMEPLAY_STATE);
    assert(state.currentTurn == BLACK);
    assert(state.result == RESULT_NONE);
    assert(state.gameOver == 0);
}

/* Run the Phase E control-flow integration tests. */
int main(void) {
    test_process_event_applies_valid_move();
    test_process_event_rejects_illegal_move();
    test_process_event_undo_restores_position();
    test_process_event_hvc_undo_returns_to_previous_white_human_turn();
    test_process_event_hvc_undo_returns_to_previous_black_human_turn();
    test_process_event_hvc_black_opening_undo_unavailable();
    test_process_event_auto_promotes_to_queen();
    test_process_event_applies_castling();
    test_process_event_applies_en_passant();
    test_process_event_timer_expiry_passes_turn();
    return 0;
}
