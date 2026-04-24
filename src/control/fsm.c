#include "system/fsm.h"

#include <stddef.h>

#include "gameplay/endgame.h"
#include "gameplay/execution.h"
#include "gameplay/validation.h"
#include "log/log.h"
#include "time/clock.h"
#include "turn/turn.h"
#include "turn/turn_timer.h"

/*
 * Alignment assumptions for future extensions:
 * - This file is the internal event/state execution engine behind the
 *   controller-facing runtime layer.
 * - fsm.h remains exported as a compatibility surface for existing tests and
 *   temporary frontends, but future UI work should prefer controller.h.
 * - This file owns system-state transitions and gameplay side effects, not
 *   event polling or queue priority policy.
 * - Helpers that are not declared in headers stay private to the FSM implementation.
 */

typedef struct {
    SystemState from;
    SystemState to;
} Transition;

static const Transition VALID_TRANSITIONS[] = {
    {INIT_STATE, MAIN_MENU_STATE},
    {MAIN_MENU_STATE, GAME_MODE_SELECTION_STATE},
    {MAIN_MENU_STATE, EXIT_STATE},
    {GAME_MODE_SELECTION_STATE, GAME_SETUP_STATE},
    {GAME_MODE_SELECTION_STATE, MAIN_MENU_STATE},
    {GAME_MODE_SELECTION_STATE, EXIT_STATE},
    {GAME_SETUP_STATE, GAMEPLAY_STATE},
    {GAME_SETUP_STATE, GAME_MODE_SELECTION_STATE},
    {GAME_SETUP_STATE, EXIT_STATE},
    {GAMEPLAY_STATE, GAME_TERMINATION_STATE},
    {GAMEPLAY_STATE, EXIT_STATE},
    {GAME_TERMINATION_STATE, END_GAME_MENU_STATE},
    {GAME_TERMINATION_STATE, EXIT_STATE},
    {END_GAME_MENU_STATE, GAME_MODE_SELECTION_STATE},
    {END_GAME_MENU_STATE, MAIN_MENU_STATE},
    {END_GAME_MENU_STATE, EXIT_STATE}
};

/* Check whether one state transition is permitted by the FSM table. */
static int transition_is_allowed(SystemState from, SystemState to) {
    int index;

    if (from == to) {
        return 1;
    }

    for (index = 0; index < (int)(sizeof(VALID_TRANSITIONS) / sizeof(VALID_TRANSITIONS[0])); ++index) {
        if (VALID_TRANSITIONS[index].from == from && VALID_TRANSITIONS[index].to == to) {
            return 1;
        }
    }

    return 0;
}

/* Return the single human-controlled color for human-vs-computer mode. */
static Color human_color_for_undo(const GameState *state) {
    if (state == NULL || state->config.mode != MODE_HUMAN_VS_COMPUTER) {
        return EMPTY_COLOR;
    }

    if (state->players[WHITE].type == HUMAN && state->players[BLACK].type == AI) {
        return WHITE;
    }

    if (state->players[WHITE].type == AI && state->players[BLACK].type == HUMAN) {
        return BLACK;
    }

    return EMPTY_COLOR;
}

/* Decide which historical ply count undo should restore before mutating state. */
static int find_undo_target_history_count(const GameState *state, int *targetCount) {
    int candidateCount;
    Color humanColor;

    if (state == NULL || targetCount == NULL || state->moveHistory.count <= 0) {
        return 1;
    }

    if (state->config.mode == MODE_HUMAN_VS_HUMAN) {
        if (state->moveHistory.count >= 2) {
            *targetCount = state->moveHistory.count - 2;
        } else {
            *targetCount = 0;
        }
        return 0;
    }

    if (state->config.mode != MODE_HUMAN_VS_COMPUTER) {
        *targetCount = state->moveHistory.count - 1;
        return 0;
    }

    humanColor = human_color_for_undo(state);
    if (humanColor == EMPTY_COLOR) {
        return 1;
    }

    for (candidateCount = state->moveHistory.count - 1; candidateCount >= 0; --candidateCount) {
        Color nextTurn = ((candidateCount % 2) == 0) ? WHITE : BLACK;

        if (nextTurn == humanColor) {
            *targetCount = candidateCount;
            return 0;
        }
    }

    return 1;
}

/* Apply one already validated move and update logging, timer, and endgame state. */
static int apply_resolved_move(GameState *state, Move move) {
    if (state == NULL) {
        return 1;
    }

    if (!validateMove(state, move)) {
        return 1;
    }

    if (state->moveHistory.count >= MAX_MOVES) {
        setGameResult(state, RESULT_DRAW);
        return transitionState(state, GAME_TERMINATION_STATE);
    }

    if (applyMove(state, move) != 0) {
        return 1;
    }

    if (logMove(state, move) != 0) {
        return 1;
    }

    if (detectGameResult(state) != 0) {
        return transitionState(state, GAME_TERMINATION_STATE);
    }

    return resetTurnTimer(state);
}

/* Handle an already resolved move while gameplay is active. */
static int handle_resolved_move(GameState *state, Move move) {
    return apply_resolved_move(state, move);
}

/* Handle undo by restoring board, timers, and persisted move log state. */
static int handle_undo(GameState *state) {
    int targetHistoryCount;

    if (find_undo_target_history_count(state, &targetHistoryCount) != 0) {
        return 1;
    }

    while (state->moveHistory.count > targetHistoryCount) {
        if (undoMove(state) != 0) {
            return 1;
        }
    }

    if (resetTurnTimer(state) != 0) {
        return 1;
    }

    return rebuildLogFromHistory(state);
}

/* Handle gameplay-state events, including move application and timeouts. The
 * controller decides when a timeout event is emitted; the FSM decides how that
 * event mutates gameplay state. */
static int handle_gameplay_event(GameState *state, Event event) {
    if (state == NULL) {
        return 1;
    }

    switch (event.type) {
        case EVENT_MOVE_INPUT:
            return 1;
        case EVENT_PLAYER_MOVE:
        case EVENT_AI_MOVE:
            return handle_resolved_move(state, event.data.move);
        case EVENT_UNDO:
            return handle_undo(state);
        case EVENT_TIMER_EXPIRED:
            if (switchTurn(state) != 0) {
                return 1;
            }
            setGameResult(state, RESULT_NONE);
            return resetTurnTimer(state);
        case EVENT_LEAVE_GAME:
            setGameOver(state);
            return transitionState(state, GAME_TERMINATION_STATE);
        case EVENT_HINT:
            /* Hints are a read-only controller query. This legacy event remains
             * rejected so gameplay state cannot be changed by a hint request. */
            return 1;
        case EVENT_EXIT_PROGRAM:
            return transitionState(state, EXIT_STATE);
        default:
            return 0;
    }
}

/* EVENT_NONE is the compatibility payload normally used to advance out of INIT. */
static int handle_init_state(GameState *state, Event event) {
    (void)event;
    return transitionState(state, MAIN_MENU_STATE);
}

/* Handle only main-menu events. Event polling and queue priority live in the controller. */
static int handle_main_menu_event(GameState *state, Event event) {
    switch (event.type) {
        case EVENT_NEW_GAME:
            return transitionState(state, GAME_MODE_SELECTION_STATE);
        case EVENT_EXIT_PROGRAM:
            return transitionState(state, EXIT_STATE);
        default:
            return 0;
    }
}

/* Handle only game-mode selection events. */
static int handle_game_mode_selection_event(GameState *state, Event event) {
    switch (event.type) {
        case EVENT_NEW_GAME:
            return transitionState(state, GAME_SETUP_STATE);
        case EVENT_BACK:
            return transitionState(state, MAIN_MENU_STATE);
        case EVENT_EXIT_PROGRAM:
            return transitionState(state, EXIT_STATE);
        default:
            return 0;
    }
}

/* Start gameplay from setup after initializing gameplay-only services. */
static int handle_game_setup_event(GameState *state, Event event) {
    switch (event.type) {
        case EVENT_NEW_GAME:
            if (initClock() != 0) {
                return 1;
            }
            if (initTurnTimer(state) != 0) {
                return 1;
            }
            if (initLog(&state->config) != 0) {
                return 1;
            }
            if (logGameStart(&state->config) != 0) {
                return 1;
            }
            return transitionState(state, GAMEPLAY_STATE);
        case EVENT_BACK:
            return transitionState(state, GAME_MODE_SELECTION_STATE);
        case EVENT_EXIT_PROGRAM:
            return transitionState(state, EXIT_STATE);
        default:
            return 0;
    }
}

/* Advance a termination state into the end-game menu after final logging. The
 * event payload is ignored for compatibility; controller normally sends EVENT_NONE. */
static int handle_game_termination_event(GameState *state, Event event) {
    (void)event;

    if (state == NULL) {
        return 1;
    }

    /* Termination should still advance even if logging was never initialized
     * in a narrow test harness path. */
    (void)logGameEnd(state);
    return transitionState(state, END_GAME_MENU_STATE);
}

/* Handle only end-game menu events. */
static int handle_end_game_menu_event(GameState *state, Event event) {
    switch (event.type) {
        case EVENT_NEW_GAME:
            return transitionState(state, GAME_MODE_SELECTION_STATE);
        case EVENT_BACK:
            return transitionState(state, MAIN_MENU_STATE);
        case EVENT_EXIT_PROGRAM:
            return transitionState(state, EXIT_STATE);
        default:
            return 0;
    }
}

/* Process one event according to the current FSM state stored in GameState.
 * This remains callable for compatibility, but it is not the preferred
 * long-term UI integration surface. */
int processEvent(GameState *state, Event event) {
    if (state == NULL) {
        return 1;
    }

    if (event.type == EVENT_FATAL_ERROR) {
        state->systemState = EXIT_STATE;
        return 0;
    }

    switch (state->systemState) {
        case INIT_STATE:
            return handle_init_state(state, event);
        case MAIN_MENU_STATE:
            return handle_main_menu_event(state, event);
        case GAME_MODE_SELECTION_STATE:
            return handle_game_mode_selection_event(state, event);
        case GAME_SETUP_STATE:
            return handle_game_setup_event(state, event);
        case GAMEPLAY_STATE:
            return handle_gameplay_event(state, event);
        case GAME_TERMINATION_STATE:
            return handle_game_termination_event(state, event);
        case END_GAME_MENU_STATE:
            return handle_end_game_menu_event(state, event);
        case EXIT_STATE:
            return 0;
        default:
            return 1;
    }
}

/* Attempt one explicit state transition if the FSM table allows it. This
 * helper remains exported mainly for compatibility-oriented tests. */
int transitionState(GameState *state, SystemState newState) {
    if (state == NULL) {
        return 1;
    }

    if (!transition_is_allowed(state->systemState, newState)) {
        return 1;
    }

    state->systemState = newState;
    return 0;
}
