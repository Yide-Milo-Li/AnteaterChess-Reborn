#include <assert.h>
#include <stddef.h>

#include "core/board.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/move.h"
#include "core/movelist.h"
#include "core/piece.h"
#include "gameplay/move_resolver.h"
#include "input/move_request.h"
#include "system/controller.h"
#include "system/controller_driver.h"
#include "system/event.h"
#include "system/system_state.h"

/* Build a fresh gameplay-ready controller through the public controller
 * startup path. */
static Controller fresh_gameplay_controller(void) {
    Controller controller;
    GameConfig config;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    assert(controller.state.systemState == GAMEPLAY_STATE);
    return controller;
}

/* Enqueue one external event and process exactly one controller tick. */
static int enqueue_and_tick(Controller *controller, Event event) {
    Event processedEvent;

    assert(controller != NULL);
    if (controllerEnqueueEvent(controller, event) != 0) {
        return 1;
    }

    return controllerTick(controller, &processedEvent);
}

static int submit_request_and_tick(Controller *controller, Position from, Position to) {
    MoveRequest request;
    Move move;

    assert(controller != NULL);
    if (createMoveRequest(&request, from, to, PROMOTION_CHOICE_QUEEN) != 0) {
        return 1;
    }

    if (resolveMoveRequest(&controller->state, request, &move) != 0) {
        return 1;
    }

    return enqueue_and_tick(controller, createPlayerMoveEvent(move));
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

/* Check that a valid move request is applied through the public controller
 * path. */
static void test_controller_applies_valid_move(void) {
    Controller controller = fresh_gameplay_controller();

    seed_simple_ant_position(&controller.state);
    assert(submit_request_and_tick(&controller,
        createPosition(6, 0), createPosition(5, 0)) == 0);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type == EMPTY_PIECE);
    assert(getPiece(&controller.state.board, createPosition(5, 0)).type == ANT);
    assert(controller.state.currentTurn == BLACK);
    assert(controller.state.moveHistory.count == 1);
}

/* Check that an illegal move request is rejected without mutating the board. */
static void test_controller_rejects_illegal_move(void) {
    Controller controller = fresh_gameplay_controller();

    seed_simple_ant_position(&controller.state);
    assert(submit_request_and_tick(&controller,
        createPosition(6, 0), createPosition(6, 1)) != 0);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type == ANT);
    assert(getPiece(&controller.state.board, createPosition(4, 0)).type == EMPTY_PIECE);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.moveHistory.count == 0);
}

/* Check that human-vs-human undo rewinds a single opening move back to the
 * initial position. */
static void test_controller_undo_restores_position(void) {
    Controller controller = fresh_gameplay_controller();

    seed_simple_ant_position(&controller.state);
    controller.state.config.mode = MODE_HUMAN_VS_HUMAN;
    assert(submit_request_and_tick(&controller,
        createPosition(6, 0), createPosition(5, 0)) == 0);
    assert(enqueue_and_tick(&controller, createUndoEvent()) == 0);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type == ANT);
    assert(getPiece(&controller.state.board, createPosition(5, 0)).type == EMPTY_PIECE);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.moveHistory.count == 0);
}

/* Check that human-vs-human undo rewinds a full round instead of stopping
 * after only Black's latest move. */
static void test_controller_hvh_undo_rewinds_full_round(void) {
    Controller controller = fresh_gameplay_controller();

    seed_empty_gameplay_position(&controller.state, WHITE);
    controller.state.config.mode = MODE_HUMAN_VS_HUMAN;
    setPiece(&controller.state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&controller.state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&controller.state.board, createPosition(6, 0), createPiece(ANT, WHITE));
    setPiece(&controller.state.board, createPosition(1, 0), createPiece(ANT, BLACK));

    assert(submit_request_and_tick(&controller,
        createPosition(6, 0), createPosition(5, 0)) == 0);
    assert(submit_request_and_tick(&controller,
        createPosition(1, 0), createPosition(2, 0)) == 0);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.moveHistory.count == 2);

    assert(enqueue_and_tick(&controller, createUndoEvent()) == 0);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type == ANT);
    assert(getPiece(&controller.state.board, createPosition(5, 0)).type == EMPTY_PIECE);
    assert(getPiece(&controller.state.board, createPosition(1, 0)).type == ANT);
    assert(getPiece(&controller.state.board, createPosition(2, 0)).type == EMPTY_PIECE);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.moveHistory.count == 0);
}

/* Check that human-vs-computer undo rewinds White games to the previous human
 * turn instead of stopping on the AI turn. */
static void test_controller_hvc_undo_returns_to_previous_white_human_turn(void) {
    Controller controller = fresh_gameplay_controller();
    Move aiMove;

    seed_empty_gameplay_position(&controller.state, WHITE);
    seed_human_vs_computer_players(&controller.state, WHITE);
    setPiece(&controller.state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&controller.state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&controller.state.board, createPosition(6, 0), createPiece(ANT, WHITE));
    setPiece(&controller.state.board, createPosition(1, 0), createPiece(ANT, BLACK));

    assert(submit_request_and_tick(&controller,
        createPosition(6, 0), createPosition(5, 0)) == 0);

    aiMove = createMove(createPosition(1, 0), createPosition(2, 0), createPiece(ANT, BLACK));
    assert(enqueue_and_tick(&controller, createAIMoveEvent(aiMove)) == 0);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.moveHistory.count == 2);

    assert(enqueue_and_tick(&controller, createUndoEvent()) == 0);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type == ANT);
    assert(getPiece(&controller.state.board, createPosition(5, 0)).type == EMPTY_PIECE);
    assert(getPiece(&controller.state.board, createPosition(1, 0)).type == ANT);
    assert(getPiece(&controller.state.board, createPosition(2, 0)).type == EMPTY_PIECE);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.moveHistory.count == 0);
}

/* Check that human-vs-computer undo rewinds Black games to the previous human
 * turn after White's opening AI move. */
static void test_controller_hvc_undo_returns_to_previous_black_human_turn(void) {
    Controller controller = fresh_gameplay_controller();
    Move aiOpeningMove;
    Move aiReplyMove;

    seed_empty_gameplay_position(&controller.state, WHITE);
    seed_human_vs_computer_players(&controller.state, BLACK);
    setPiece(&controller.state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&controller.state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&controller.state.board, createPosition(6, 0), createPiece(ANT, WHITE));
    setPiece(&controller.state.board, createPosition(1, 0), createPiece(ANT, BLACK));

    aiOpeningMove = createMove(createPosition(6, 0), createPosition(5, 0), createPiece(ANT, WHITE));
    assert(enqueue_and_tick(&controller, createAIMoveEvent(aiOpeningMove)) == 0);

    assert(submit_request_and_tick(&controller,
        createPosition(1, 0), createPosition(2, 0)) == 0);

    aiReplyMove = createMove(createPosition(5, 0), createPosition(4, 0), createPiece(ANT, WHITE));
    assert(enqueue_and_tick(&controller, createAIMoveEvent(aiReplyMove)) == 0);
    assert(controller.state.currentTurn == BLACK);
    assert(controller.state.moveHistory.count == 3);

    assert(enqueue_and_tick(&controller, createUndoEvent()) == 0);
    assert(getPiece(&controller.state.board, createPosition(5, 0)).type == ANT);
    assert(getPiece(&controller.state.board, createPosition(4, 0)).type == EMPTY_PIECE);
    assert(getPiece(&controller.state.board, createPosition(1, 0)).type == ANT);
    assert(getPiece(&controller.state.board, createPosition(2, 0)).type == EMPTY_PIECE);
    assert(controller.state.currentTurn == BLACK);
    assert(controller.state.moveHistory.count == 1);
}

/* Check that Black cannot undo White's opening AI move before Black has taken
 * a turn. */
static void test_controller_hvc_black_opening_undo_unavailable(void) {
    Controller controller = fresh_gameplay_controller();
    Move aiOpeningMove;

    seed_empty_gameplay_position(&controller.state, WHITE);
    seed_human_vs_computer_players(&controller.state, BLACK);
    setPiece(&controller.state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&controller.state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&controller.state.board, createPosition(6, 0), createPiece(ANT, WHITE));
    setPiece(&controller.state.board, createPosition(1, 0), createPiece(ANT, BLACK));

    aiOpeningMove = createMove(createPosition(6, 0), createPosition(5, 0), createPiece(ANT, WHITE));
    assert(enqueue_and_tick(&controller, createAIMoveEvent(aiOpeningMove)) == 0);

    assert(enqueue_and_tick(&controller, createUndoEvent()) != 0);
    assert(getPiece(&controller.state.board, createPosition(5, 0)).type == ANT);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type == EMPTY_PIECE);
    assert(getPiece(&controller.state.board, createPosition(1, 0)).type == ANT);
    assert(controller.state.currentTurn == BLACK);
    assert(controller.state.moveHistory.count == 1);
}

/* Check that public move requests can promote to queen explicitly. */
static void test_controller_auto_promotes_to_queen(void) {
    Controller controller = fresh_gameplay_controller();

    seed_empty_gameplay_position(&controller.state, WHITE);
    setPiece(&controller.state.board, createPosition(1, 2), createPiece(ANT, WHITE));
    setPiece(&controller.state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&controller.state.board, createPosition(0, 5), createPiece(KING, BLACK));

    assert(submit_request_and_tick(&controller,
        createPosition(1, 2), createPosition(0, 2)) == 0);
    assert(getPiece(&controller.state.board, createPosition(0, 2)).type == QUEEN);
    assert(getPiece(&controller.state.board, createPosition(0, 2)).color == WHITE);
}

/* Check that castling resolves correctly from the existing from/to request
 * input format. */
static void test_controller_applies_castling(void) {
    Controller controller = fresh_gameplay_controller();

    seed_empty_gameplay_position(&controller.state, WHITE);
    setPiece(&controller.state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&controller.state.board, createPosition(7, 9), createPiece(ROOK, WHITE));
    setPiece(&controller.state.board, createPosition(0, 5), createPiece(KING, BLACK));

    assert(submit_request_and_tick(&controller,
        createPosition(7, 5), createPosition(7, 7)) == 0);
    assert(getPiece(&controller.state.board, createPosition(7, 7)).type == KING);
    assert(getPiece(&controller.state.board, createPosition(7, 6)).type == ROOK);
}

/* Check that en passant resolves correctly from the existing from/to request
 * input format and latest move history. */
static void test_controller_applies_en_passant(void) {
    Controller controller = fresh_gameplay_controller();

    seed_empty_gameplay_position(&controller.state, WHITE);
    setPiece(&controller.state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&controller.state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&controller.state.board, createPosition(3, 4), createPiece(ANT, WHITE));
    setPiece(&controller.state.board, createPosition(3, 5), createPiece(ANT, BLACK));
    push_history_move(&controller.state,
        createMove(createPosition(1, 5), createPosition(3, 5), createPiece(ANT, BLACK)));

    assert(submit_request_and_tick(&controller,
        createPosition(3, 4), createPosition(2, 5)) == 0);
    assert(getPiece(&controller.state.board, createPosition(2, 5)).type == ANT);
    assert(getPiece(&controller.state.board, createPosition(3, 5)).type == EMPTY_PIECE);
}

/* Check that timer expiry skips the active turn and keeps gameplay running. */
static void test_controller_timer_expiry_passes_turn(void) {
    Controller controller = fresh_gameplay_controller();

    seed_empty_gameplay_position(&controller.state, WHITE);
    setPiece(&controller.state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&controller.state.board, createPosition(0, 5), createPiece(KING, BLACK));

    assert(enqueue_and_tick(&controller, createSystemEvent(EVENT_TIMER_EXPIRED)) == 0);
    assert(controller.state.systemState == GAMEPLAY_STATE);
    assert(controller.state.currentTurn == BLACK);
    assert(controller.state.result == RESULT_NONE);
    assert(controller.state.gameOver == 0);
}

/* Run the control-flow integration tests through the public controller API. */
int main(void) {
    test_controller_applies_valid_move();
    test_controller_rejects_illegal_move();
    test_controller_undo_restores_position();
    test_controller_hvh_undo_rewinds_full_round();
    test_controller_hvc_undo_returns_to_previous_white_human_turn();
    test_controller_hvc_undo_returns_to_previous_black_human_turn();
    test_controller_hvc_black_opening_undo_unavailable();
    test_controller_auto_promotes_to_queen();
    test_controller_applies_castling();
    test_controller_applies_en_passant();
    test_controller_timer_expiry_passes_turn();
    return 0;
}
