#include "system/fsm.h"

#include <stddef.h>

#include "gameplay/endgame.h"
#include "gameplay/execution.h"
#include "gameplay/movegen.h"
#include "gameplay/validation.h"
#include "log/log.h"
#include "time/clock.h"
#include "turn/turn_timer.h"

/*
 * Alignment assumptions for future extensions:
 * - fsm.h is the truth source for public FSM entrypoints and transition helpers.
 * - This file owns system-state transitions, not event polling or queue priority policy.
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

/* Build the exact legal move candidate that matches one move command. */
static int find_command_move(const GameState *state, Command command, Move *resolvedMove) {
    MoveList candidates;
    Piece movingPiece;
    Move *promotionQueenCandidate;
    int matchingCount;
    int allMatchesArePromotions;
    int index;

    if (state == NULL || resolvedMove == NULL || command.type != CMD_MOVE) {
        return 1;
    }

    if (validateSelection(state, command.from) != SELECT_VALID || !isValidPosition(command.to)) {
        return 1;
    }

    movingPiece = getPiece(&state->board, command.from);
    if (movingPiece.type == EMPTY_PIECE) {
        return 1;
    }

    if (generateLegalMovesForPosition(state, command.from, &candidates) != 0) {
        return 1;
    }

    promotionQueenCandidate = NULL;
    matchingCount = 0;
    allMatchesArePromotions = 1;
    for (index = 0; index < getMoveCount(&candidates); ++index) {
        Move *candidate = getMove(&candidates, index);

        if (candidate != NULL
            && positionEqual(candidate->from, command.from)
            && positionEqual(candidate->to, command.to)
            && candidate->movedPiece.type == movingPiece.type
            && candidate->movedPiece.color == movingPiece.color) {
            ++matchingCount;
            if (candidate->specialType == PROMOTION_QUEEN) {
                promotionQueenCandidate = candidate;
            } else if (candidate->specialType != PROMOTION_ROOK
                && candidate->specialType != PROMOTION_BISHOP
                && candidate->specialType != PROMOTION_KNIGHT) {
                allMatchesArePromotions = 0;
            }

            if (matchingCount == 1) {
                *resolvedMove = *candidate;
            }
        }
    }

    if (matchingCount == 1) {
        return 0;
    }

    if (matchingCount > 1 && allMatchesArePromotions && promotionQueenCandidate != NULL) {
        *resolvedMove = *promotionQueenCandidate;
        return 0;
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

/* Handle a player-entered move command while gameplay is active. */
static int handle_move_input(GameState *state, Command command) {
    Move move;

    if (find_command_move(state, command, &move) != 0) {
        return 1;
    }

    return apply_resolved_move(state, move);
}

/* Handle an AI-selected move while gameplay is active. */
static int handle_ai_move(GameState *state, Move move) {
    return apply_resolved_move(state, move);
}

/* Handle undo by restoring board, timers, and persisted move log state. */
static int handle_undo(GameState *state) {
    if (undoMove(state) != 0) {
        return 1;
    }

    if (resetTurnTimer(state) != 0) {
        return 1;
    }

    return rebuildLogFromHistory(state);
}

/* Handle gameplay-state events, including move application and timeouts. */
static int handle_gameplay_event(GameState *state, Event event) {
    if (state == NULL) {
        return 1;
    }

    switch (event.type) {
        case EVENT_MOVE_INPUT:
            return handle_move_input(state, event.data.command);
        case EVENT_AI_MOVE:
            return handle_ai_move(state, event.data.move);
        case EVENT_UNDO:
            return handle_undo(state);
        case EVENT_TIMER_EXPIRED:
            if (state->currentTurn == WHITE) {
                setGameResult(state, RESULT_BLACK_WIN);
            } else {
                setGameResult(state, RESULT_WHITE_WIN);
            }
            return transitionState(state, GAME_TERMINATION_STATE);
        case EVENT_LEAVE_GAME:
            setGameOver(state);
            return transitionState(state, GAME_TERMINATION_STATE);
        case EVENT_HINT:
            return 1;
        case EVENT_EXIT_PROGRAM:
            return transitionState(state, EXIT_STATE);
        default:
            return 0;
    }
}

/* Advance a termination state into the end-game menu after final logging. */
static int handle_game_termination(GameState *state) {
    if (state == NULL) {
        return 1;
    }

    /* Termination should still advance even if logging was never initialized
     * in a narrow test harness path. */
    (void)logGameEnd(state);
    return transitionState(state, END_GAME_MENU_STATE);
}

/* Process one event according to the current FSM state stored in GameState. */
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
            return transitionState(state, MAIN_MENU_STATE);
        case MAIN_MENU_STATE:
            switch (event.type) {
                case EVENT_NEW_GAME:
                    return transitionState(state, GAME_MODE_SELECTION_STATE);
                case EVENT_EXIT_PROGRAM:
                    return transitionState(state, EXIT_STATE);
                default:
                    return 0;
            }
        case GAME_MODE_SELECTION_STATE:
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
        case GAME_SETUP_STATE:
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
        case GAMEPLAY_STATE:
            return handle_gameplay_event(state, event);
        case GAME_TERMINATION_STATE:
            return handle_game_termination(state);
        case END_GAME_MENU_STATE:
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
        case EXIT_STATE:
            return 0;
        default:
            return 1;
    }
}

/* Attempt one explicit state transition if the FSM table allows it. */
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
