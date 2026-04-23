#include <assert.h>
#include <stddef.h>

#include "core/board.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/piece.h"
#include "gameplay/validation.h"
#include "system/controller.h"
#include "system/event.h"
#include "system/system_state.h"

/* Build a fresh controller using the public default configuration. */
static Controller fresh_controller(void) {
    Controller controller;
    GameConfig config;

    initDefaultGameConfig(&config);
    initController(&controller, &config);
    return controller;
}

/* Rebuild a minimal gameplay position directly on one controller-owned state. */
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

/* Prepare a minimal gameplay position where one white ant has one legal move. */
static void seed_simple_ant_position(GameState *state) {
    assert(state != NULL);
    seed_empty_gameplay_position(state, WHITE);
    setPiece(&state->board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state->board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state->board, createPosition(6, 0), createPiece(ANT, WHITE));
}

/* Check that controller initialization resets state and queue storage. */
static void test_init_controller_resets_state_and_queue(void) {
    Controller controller = fresh_controller();

    assert(controllerGetState(NULL) == NULL);
    assert(controllerGetState(&controller) == &controller.state);
    assert(controller.state.systemState == INIT_STATE);
    assert(controller.state.currentTurn == WHITE);
    assert(isControlQueueEmpty(&controller.queue));
    assert(isSystemQueueEmpty(&controller.queue));
    assert(isGameplayQueueEmpty(&controller.queue));
}

/* Check that public enqueueing routes events into the expected priority
 * subqueues. */
static void test_controller_enqueue_event_routes_by_priority(void) {
    Controller controller = fresh_controller();

    assert(controllerEnqueueEvent(&controller, createUndoEvent()) == 0);
    assert(controllerEnqueueEvent(&controller, createSystemEvent(EVENT_TIMER_EXPIRED)) == 0);
    assert(controllerEnqueueEvent(&controller, createSystemEvent(EVENT_NEW_GAME)) == 0);

    assert(dequeueControlEvent(&controller.queue).type == EVENT_NEW_GAME);
    assert(dequeueSystemEvent(&controller.queue).type == EVENT_TIMER_EXPIRED);
    assert(dequeueGameplayEvent(&controller.queue).type == EVENT_UNDO);
}

/* Check that one controller tick bootstraps INIT into the main menu with the
 * compatibility EVENT_NONE handshake. */
static void test_controller_tick_bootstraps_init_state(void) {
    Controller controller = fresh_controller();
    Event processedEvent;

    assert(controllerTick(&controller, &processedEvent) == 0);
    assert(processedEvent.type == EVENT_NONE);
    assert(controller.state.systemState == MAIN_MENU_STATE);
}

/* Check that one queued external event is processed in a single tick. */
static void test_controller_tick_processes_one_queued_event(void) {
    Controller controller = fresh_controller();
    Event processedEvent;

    assert(controllerTick(&controller, &processedEvent) == 0);
    assert(controller.state.systemState == MAIN_MENU_STATE);
    assert(controllerEnqueueEvent(&controller, createSystemEvent(EVENT_NEW_GAME)) == 0);
    assert(controllerTick(&controller, &processedEvent) == 0);
    assert(processedEvent.type == EVENT_NEW_GAME);
    assert(controller.state.systemState == GAME_MODE_SELECTION_STATE);
}

/* Check that ticking an already idle controller returns EVENT_NONE without
 * mutating state. */
static void test_controller_tick_returns_none_when_idle(void) {
    Controller controller = fresh_controller();
    Event processedEvent;

    assert(controllerTick(&controller, &processedEvent) == 0);
    assert(controller.state.systemState == MAIN_MENU_STATE);
    assert(controllerTick(&controller, &processedEvent) == 0);
    assert(processedEvent.type == EVENT_NONE);
    assert(controller.state.systemState == MAIN_MENU_STATE);
}

/* Check that run-until-idle advances the boot handshake and then stops at the
 * first UI-facing idle point. */
static void test_controller_run_until_idle_bootstraps_init_state(void) {
    Controller controller = fresh_controller();

    assert(controllerRunUntilIdle(&controller) == 0);
    assert(controller.state.systemState == MAIN_MENU_STATE);
}

/* Check that run-until-idle completes the termination handshake into the
 * end-game menu. */
static void test_controller_run_until_idle_completes_termination_handshake(void) {
    Controller controller = fresh_controller();

    controller.state.systemState = GAME_TERMINATION_STATE;
    controller.state.gameOver = 1;
    controller.state.result = RESULT_TERMINATED_BY_USER;
    assert(controllerRunUntilIdle(&controller) == 0);
    assert(controller.state.systemState == END_GAME_MENU_STATE);
}

/* Check that configured-game startup reaches gameplay without auto-playing the
 * opening AI turn. */
static void test_controller_start_configured_game_enters_gameplay(void) {
    Controller controller;
    GameConfig config;
    const GameState *state;

    initDefaultGameConfig(&config);
    config.mode = MODE_HUMAN_VS_COMPUTER;
    config.playerColor = BLACK;

    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    state = controllerGetState(&controller);
    assert(state != NULL);
    assert(state->systemState == GAMEPLAY_STATE);
    assert(state->players[WHITE].type == AI);
    assert(state->players[BLACK].type == HUMAN);
    assert(state->currentTurn == WHITE);
    assert(state->moveHistory.count == 0);
}

/* Check that the façade-style new-game request bootstraps INIT before
 * advancing to the next UI-facing menu. */
static void test_controller_request_new_game_advances_to_mode_menu(void) {
    Controller controller = fresh_controller();

    assert(controllerRequestNewGame(&controller) == 0);
    assert(controller.state.systemState == GAME_MODE_SELECTION_STATE);
}

/* Check that the façade-style back request drains the transition to the main
 * menu. */
static void test_controller_request_back_returns_to_main_menu(void) {
    Controller controller = fresh_controller();

    assert(controllerRequestNewGame(&controller) == 0);
    assert(controllerRequestBack(&controller) == 0);
    assert(controller.state.systemState == MAIN_MENU_STATE);
}

/* Check that the façade-style exit request reaches EXIT without exposing queue
 * details to the caller. */
static void test_controller_request_exit_reaches_exit_state(void) {
    Controller controller = fresh_controller();

    assert(controllerRequestExit(&controller) == 0);
    assert(controller.state.systemState == EXIT_STATE);
}

/* Check that the façade move-submission helper applies one valid move and
 * drains follow-up controller work. */
static void test_controller_submit_move_applies_valid_move(void) {
    Controller controller;
    GameConfig config;
    Command command;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    seed_simple_ant_position(&controller.state);
    assert(createMoveCommand(&command, createPosition(6, 0),
        createPosition(5, 0)) == 0);
    assert(controllerSubmitMove(&controller, command) == 0);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type == EMPTY_PIECE);
    assert(getPiece(&controller.state.board, createPosition(5, 0)).type == ANT);
    assert(controller.state.currentTurn == BLACK);
    assert(controller.state.moveHistory.count == 1);
}

/* Check that the façade undo helper rewinds back to the previous human turn
 * and leaves the controller idle again. */
static void test_controller_request_undo_restores_position(void) {
    Controller controller;
    GameConfig config;
    Command command;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    seed_simple_ant_position(&controller.state);
    assert(createMoveCommand(&command, createPosition(6, 0),
        createPosition(5, 0)) == 0);
    assert(controllerSubmitMove(&controller, command) == 0);
    assert(controllerRequestUndo(&controller) == 0);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type == ANT);
    assert(getPiece(&controller.state.board, createPosition(5, 0)).type == EMPTY_PIECE);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.moveHistory.count == 0);
}

/* Check that the façade leave-game helper drains the termination handshake
 * into the end-game menu. */
static void test_controller_request_leave_game_reaches_endgame_menu(void) {
    Controller controller;
    GameConfig config;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    seed_simple_ant_position(&controller.state);
    assert(controllerRequestLeaveGame(&controller) == 0);
    assert(controller.state.systemState == END_GAME_MENU_STATE);
    assert(controller.state.gameOver == 1);
    assert(controller.state.result == RESULT_TERMINATED_BY_USER);
}

/* Check that the façade hint helper returns one legal move without mutating
 * controller-owned gameplay state. */
static void test_controller_get_hint_returns_move_without_mutating_state(void) {
    Controller controller;
    GameConfig config;
    GameState before;
    Move move;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    seed_simple_ant_position(&controller.state);
    before = controller.state;

    assert(controllerGetHint(&controller, &move) == 0);
    assert(validateMove(&controller.state, move) == 1);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type
        == getPiece(&before.board, createPosition(6, 0)).type);
    assert(getPiece(&controller.state.board, createPosition(5, 0)).type
        == getPiece(&before.board, createPosition(5, 0)).type);
    assert(controller.state.currentTurn == before.currentTurn);
    assert(controller.state.moveHistory.count == before.moveHistory.count);
}

/* Check that the legacy wrapper rejects null input. */
static void test_run_game_loop_rejects_null_state(void) {
    assert(runGameLoop(NULL) != 0);
}

/* Check that the legacy wrapper still bootstraps INIT into the main menu. */
static void test_run_game_loop_bootstraps_init_state(void) {
    GameState state;
    GameConfig config;

    initDefaultGameConfig(&config);
    initGameState(&state, &config);
    assert(runGameLoop(&state) == 0);
    assert(state.systemState == MAIN_MENU_STATE);
}

/* Check that the legacy wrapper still handles the termination handshake. */
static void test_run_game_loop_completes_termination_handshake(void) {
    GameState state;
    GameConfig config;

    initDefaultGameConfig(&config);
    initGameState(&state, &config);
    state.systemState = GAME_TERMINATION_STATE;
    state.gameOver = 1;
    state.result = RESULT_TERMINATED_BY_USER;
    assert(runGameLoop(&state) == 0);
    assert(state.systemState == END_GAME_MENU_STATE);
}

/* Check that the legacy wrapper exits cleanly when the state is already
 * terminal. */
static void test_run_game_loop_accepts_exit_state(void) {
    GameState state;
    GameConfig config;

    initDefaultGameConfig(&config);
    initGameState(&state, &config);
    state.systemState = EXIT_STATE;
    assert(runGameLoop(&state) == 0);
    assert(state.systemState == EXIT_STATE);
}

/* Run the controller regression suite. */
int main(void) {
    test_init_controller_resets_state_and_queue();
    test_controller_enqueue_event_routes_by_priority();
    test_controller_tick_bootstraps_init_state();
    test_controller_tick_processes_one_queued_event();
    test_controller_tick_returns_none_when_idle();
    test_controller_run_until_idle_bootstraps_init_state();
    test_controller_run_until_idle_completes_termination_handshake();
    test_controller_start_configured_game_enters_gameplay();
    test_controller_request_new_game_advances_to_mode_menu();
    test_controller_request_back_returns_to_main_menu();
    test_controller_request_exit_reaches_exit_state();
    test_controller_submit_move_applies_valid_move();
    test_controller_request_undo_restores_position();
    test_controller_request_leave_game_reaches_endgame_menu();
    test_controller_get_hint_returns_move_without_mutating_state();
    test_run_game_loop_rejects_null_state();
    test_run_game_loop_bootstraps_init_state();
    test_run_game_loop_completes_termination_handshake();
    test_run_game_loop_accepts_exit_state();
    return 0;
}
