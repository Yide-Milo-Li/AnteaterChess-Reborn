#include <assert.h>
#include <time.h>

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/move.h"
#include "time/clock.h"
#include "turn/turn.h"
#include "turn/turn_timer.h"

/*
 * Alignment assumptions for future extensions:
 * - The turn timer is a module-local per-turn countdown, not a carry-over chess clock.
 * - Resetting after switchTurn() or undo starts from config.initialTimeSeconds again.
 * - These tests assume controller/FSM will separately convert expiration into an event.
 */

/* Spin until the elapsed gameplay clock changes or a hard loop cap is reached. */
static int wait_for_elapsed_change(int baseline) {
    time_t deadline = time(NULL) + 3;

    while (time(NULL) <= deadline) {
        int current = getElapsedTimeSeconds();

        if (current != baseline) {
            return current;
        }
    }

    return baseline;
}

/* Build a game state with the supplied timer configuration. */
static GameState create_timer_state(int timerEnabled, int initialTimeSeconds) {
    GameConfig config;
    GameState state;

    initDefaultGameConfig(&config);
    config.timerEnabled = timerEnabled;
    config.initialTimeSeconds = initialTimeSeconds;
    initGameState(&state, &config);
    return state;
}

/* Verify initialization and countdown for the active player only. */
static void test_turn_timer_counts_down_only_for_active_player(void) {
    GameState state = create_timer_state(1, 2);
    int baselineElapsed;

    assert(initClock() == 0);
    assert(initTurnTimer(&state) == 0);
    assert(getRemainingTime(&state, WHITE) == 2);
    assert(getRemainingTime(&state, BLACK) == 2);

    baselineElapsed = getElapsedTimeSeconds();
    assert(wait_for_elapsed_change(baselineElapsed) != baselineElapsed);
    assert(updateTurnTimer(&state) == 0);
    assert(getRemainingTime(&state, WHITE) <= 1);
    assert(getRemainingTime(&state, BLACK) == 2);
    assert(isTimeUp(&state) == 0);
}

/* Verify turn switches and undo-style resets restart the countdown from full time. */
static void test_turn_timer_resets_after_switch_and_undo(void) {
    GameState state = create_timer_state(1, 2);
    Move move;
    int baselineElapsed;

    assert(initClock() == 0);
    assert(initTurnTimer(&state) == 0);

    baselineElapsed = getElapsedTimeSeconds();
    assert(wait_for_elapsed_change(baselineElapsed) != baselineElapsed);
    assert(updateTurnTimer(&state) == 0);
    assert(getRemainingTime(&state, WHITE) <= 1);

    assert(switchTurn(&state) == 0);
    assert(resetTurnTimer(&state) == 0);
    assert(getRemainingTime(&state, BLACK) == 2);

    move = createMove(createPosition(6, 4), createPosition(5, 4), createPiece(ANT, WHITE));
    assert(addMoveToHistory(&state, move) == 0);
    assert(removeLastMoveFromHistory(&state) == 0);
    assert(restoreTurnAfterUndo(&state) == 0);
    assert(resetTurnTimer(&state) == 0);
    assert(getRemainingTime(&state, WHITE) == 2);
}

/* Verify disabled timers stay inert and enabled timers eventually expire. */
static void test_turn_timer_disable_and_expiration_paths(void) {
    GameState disabledState = create_timer_state(0, 3);
    GameState expiringState = create_timer_state(1, 1);
    int baselineElapsed;

    assert(initClock() == 0);
    assert(initTurnTimer(&disabledState) == 0);
    baselineElapsed = getElapsedTimeSeconds();
    assert(wait_for_elapsed_change(baselineElapsed) != baselineElapsed);
    assert(updateTurnTimer(&disabledState) == 0);
    assert(isTimeUp(&disabledState) == 0);
    assert(getRemainingTime(&disabledState, WHITE) == 3);

    assert(initClock() == 0);
    assert(initTurnTimer(&expiringState) == 0);
    baselineElapsed = getElapsedTimeSeconds();
    assert(wait_for_elapsed_change(baselineElapsed) != baselineElapsed);
    assert(updateTurnTimer(&expiringState) == 0);
    assert(isTimeUp(&expiringState) == 1);
    assert(getRemainingTime(&expiringState, WHITE) == 0);
}

/* Run the per-turn timer regression suite. */
int main(void) {
    test_turn_timer_counts_down_only_for_active_player();
    test_turn_timer_resets_after_switch_and_undo();
    test_turn_timer_disable_and_expiration_paths();
    return 0;
}
