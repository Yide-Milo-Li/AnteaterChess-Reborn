#include <assert.h>

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "system/event.h"
#include "system/fsm.h"
#include "system/system_state.h"

/* Build a fresh GameState whose public systemState field drives the FSM. */
static GameState fresh_state(void) {
    GameConfig config;
    GameState state;

    initDefaultGameConfig(&config);
    initGameState(&state, &config);
    return state;
}

/* Check the normal forward path from boot to the end-game menu. */
static void test_forward_state_progression(void) {
    GameState state = fresh_state();

    assert(state.systemState == INIT_STATE);
    assert(processEvent(&state, createSystemEvent(EVENT_NONE)) == 0);
    assert(state.systemState == MAIN_MENU_STATE);
    assert(processEvent(&state, createSystemEvent(EVENT_NEW_GAME)) == 0);
    assert(state.systemState == GAME_MODE_SELECTION_STATE);
    assert(processEvent(&state, createSystemEvent(EVENT_NEW_GAME)) == 0);
    assert(state.systemState == GAME_SETUP_STATE);
    assert(processEvent(&state, createSystemEvent(EVENT_NEW_GAME)) == 0);
    assert(state.systemState == GAMEPLAY_STATE);
    assert(processEvent(&state, createSystemEvent(EVENT_LEAVE_GAME)) == 0);
    assert(state.systemState == GAME_TERMINATION_STATE);
    assert(state.result == RESULT_TERMINATED_BY_USER);
    assert(processEvent(&state, createSystemEvent(EVENT_NONE)) == 0);
    assert(state.systemState == END_GAME_MENU_STATE);
}

/* Check that explicit transition validation rejects illegal jumps. */
static void test_transition_validation(void) {
    GameState state = fresh_state();

    assert(transitionState(&state, GAMEPLAY_STATE) != 0);
    assert(state.systemState == INIT_STATE);

    assert(processEvent(&state, createSystemEvent(EVENT_NONE)) == 0);
    assert(transitionState(&state, GAMEPLAY_STATE) != 0);
    assert(state.systemState == MAIN_MENU_STATE);
}

/* Check that timer expiry skips the active turn while fatal errors still force
 * terminal exit. */
static void test_timer_expiry_and_fatal_error_paths(void) {
    GameState state = fresh_state();

    assert(processEvent(&state, createSystemEvent(EVENT_NONE)) == 0);
    assert(processEvent(&state, createSystemEvent(EVENT_NEW_GAME)) == 0);
    assert(processEvent(&state, createSystemEvent(EVENT_NEW_GAME)) == 0);
    assert(processEvent(&state, createSystemEvent(EVENT_NEW_GAME)) == 0);
    assert(state.systemState == GAMEPLAY_STATE);

    state.currentTurn = WHITE;
    assert(processEvent(&state, createSystemEvent(EVENT_TIMER_EXPIRED)) == 0);
    assert(state.systemState == GAMEPLAY_STATE);
    assert(state.currentTurn == BLACK);
    assert(state.result == RESULT_NONE);
    assert(state.gameOver == 0);

    state = fresh_state();
    assert(processEvent(&state, createFatalErrorEvent(ERR_FATAL)) == 0);
    assert(state.systemState == EXIT_STATE);
}

/* Run the Phase E FSM regression suite. */
int main(void) {
    test_forward_state_progression();
    test_transition_validation();
    test_timer_expiry_and_fatal_error_paths();
    return 0;
}
