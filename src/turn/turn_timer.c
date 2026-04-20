#include "turn/turn_timer.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "time/clock.h"

/*
 * Alignment assumptions for future extensions:
 * - Turn-timer runtime state is intentionally module-local and not stored in GameState.
 * - Every new turn starts from config.initialTimeSeconds; there is no carry-over clock.
 * - Controller/FSM owns emitting EVENT_TIMER_EXPIRED after this module reports expiration.
 */

static Color timedPlayer = EMPTY_COLOR;
static int64_t turnStartElapsedSeconds = 0;
static int configuredTurnLengthSeconds = 0;
static int timerEnabled = 0;
static int timerExpired = 0;
static int timerInitialized = 0;

/* Check whether the supplied state can drive turn-timer operations. */
static int validate_timer_state(const GameState *state) {
    if (state == NULL) {
        return 1;
    }

    if (state->currentTurn != WHITE && state->currentTurn != BLACK) {
        return 1;
    }

    return 0;
}

/* Return the current countdown value for the actively timed player. */
static int compute_active_remaining_time(void) {
    int64_t elapsedSinceTurnStart;
    int64_t remaining;

    elapsedSinceTurnStart = getElapsedTimeSeconds() - turnStartElapsedSeconds;
    if (elapsedSinceTurnStart < 0) {
        elapsedSinceTurnStart = 0;
    }

    remaining = configuredTurnLengthSeconds - elapsedSinceTurnStart;
    if (remaining < 0) {
        remaining = 0;
    }

    if (remaining > INT_MAX) {
        return INT_MAX;
    }

    return (int) remaining;
}

/* Start a fresh per-turn countdown for the player whose turn is active now. */
int initTurnTimer(GameState *state) {
    if (validate_timer_state(state) != 0) {
        return 1;
    }

    configuredTurnLengthSeconds = state->config.initialTimeSeconds;
    timerEnabled = state->config.timerEnabled ? 1 : 0;
    timedPlayer = state->currentTurn;
    turnStartElapsedSeconds = getElapsedTimeSeconds();
    timerExpired = 0;
    timerInitialized = 1;
    return 0;
}

/* Refresh the expiration flag for the currently active per-turn countdown. */
int updateTurnTimer(GameState *state) {
    if (validate_timer_state(state) != 0 || !timerInitialized) {
        return 1;
    }

    if (!timerEnabled) {
        timerExpired = 0;
        timedPlayer = state->currentTurn;
        return 0;
    }

    if (state->currentTurn != timedPlayer) {
        return 1;
    }

    timerExpired = (compute_active_remaining_time() <= 0);
    return 0;
}

/* Report whether the active per-turn countdown has expired. */
int isTimeUp(const GameState *state) {
    if (validate_timer_state(state) != 0 || !timerInitialized) {
        return -1;
    }

    if (!timerEnabled) {
        return 0;
    }

    return timerExpired;
}

/* Return the active player's remaining time or the full configured reset time. */
int getRemainingTime(const GameState *state, Color playerColor) {
    if (validate_timer_state(state) != 0 || !timerInitialized) {
        return -1;
    }

    if (playerColor != WHITE && playerColor != BLACK) {
        return -1;
    }

    if (!timerEnabled) {
        return state->config.initialTimeSeconds;
    }

    if (playerColor != state->currentTurn) {
        return state->config.initialTimeSeconds;
    }

    return compute_active_remaining_time();
}

/* Restart the per-turn countdown from the configured full value. */
int resetTurnTimer(GameState *state) {
    if (validate_timer_state(state) != 0 || !timerInitialized) {
        return 1;
    }

    timedPlayer = state->currentTurn;
    configuredTurnLengthSeconds = state->config.initialTimeSeconds;
    turnStartElapsedSeconds = getElapsedTimeSeconds();
    timerExpired = 0;
    timerEnabled = state->config.timerEnabled ? 1 : 0;
    return 0;
}
