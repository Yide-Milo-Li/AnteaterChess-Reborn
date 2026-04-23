#include <assert.h>
#include <stddef.h>

#include "core/gameconfig.h"
#include "core/gamestate.h"
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
    test_run_game_loop_rejects_null_state();
    test_run_game_loop_bootstraps_init_state();
    test_run_game_loop_completes_termination_handshake();
    test_run_game_loop_accepts_exit_state();
    return 0;
}
