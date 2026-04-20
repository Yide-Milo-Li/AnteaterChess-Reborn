#include <assert.h>

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "system/controller.h"
#include "system/system_state.h"

/* Build a fresh state for controller-level black-box tests. */
static GameState fresh_state(void) {
    GameConfig config;
    GameState state;

    initDefaultGameConfig(&config);
    initGameState(&state, &config);
    return state;
}

/* Check that the public controller entrypoint rejects a null state. */
static void test_run_game_loop_rejects_null_state(void) {
    assert(runGameLoop(NULL) != 0);
}

/* Check that the controller performs the boot handshake and then idles cleanly. */
static void test_run_game_loop_bootstraps_init_state(void) {
    GameState state = fresh_state();

    assert(state.systemState == INIT_STATE);
    assert(runGameLoop(&state) == 0);
    assert(state.systemState == MAIN_MENU_STATE);
}

/* Check that a pending termination state is advanced to the end-game menu. */
static void test_run_game_loop_completes_termination_handshake(void) {
    GameState state = fresh_state();

    state.systemState = GAME_TERMINATION_STATE;
    state.gameOver = 1;
    state.result = RESULT_TERMINATED_BY_USER;
    assert(runGameLoop(&state) == 0);
    assert(state.systemState == END_GAME_MENU_STATE);
}

/* Check that the loop exits immediately when the state is already terminal. */
static void test_run_game_loop_accepts_exit_state(void) {
    GameState state = fresh_state();

    state.systemState = EXIT_STATE;
    assert(runGameLoop(&state) == 0);
    assert(state.systemState == EXIT_STATE);
}

/* Run the Phase E controller regression suite. */
int main(void) {
    test_run_game_loop_rejects_null_state();
    test_run_game_loop_bootstraps_init_state();
    test_run_game_loop_completes_termination_handshake();
    test_run_game_loop_accepts_exit_state();
    return 0;
}
