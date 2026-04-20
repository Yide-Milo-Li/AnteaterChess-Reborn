#include "system/fsm.h"

#include <stddef.h>

#include "gameplay/endgame.h"
#include "system/controller.h"

enum {
    MOVE_PIPELINE_OK                 = 0,
    MOVE_PIPELINE_ERR_NULL_ARG       = 1,
    MOVE_PIPELINE_ERR_INVALID_CMD    = 2,
    MOVE_PIPELINE_ERR_ILLEGAL_MOVE   = 3,
    MOVE_PIPELINE_ERR_APPLY_FAILED   = 4
};
int runAIMovePipeline   (GameState *state, Move move,      EventQueue *outQueue);
int runInputMovePipeline(GameState *state, Command cmd,    EventQueue *outQueue);
int runUndoPipeline     (GameState *state,                 EventQueue *outQueue);

int logGameStart(const GameState *state);
int logGameEnd  (const GameState *state);

/* Internal state */
static SystemState g_systemState = INIT_STATE;

/* Valid-transitions table */

typedef struct {
    SystemState from;
    SystemState to;
} Transition;

static const Transition VALID[] = {
    /* Initialization */
    { INIT_STATE,                MAIN_MENU_STATE            },

    /* Main menu */
    { MAIN_MENU_STATE,           GAME_MODE_SELECTION_STATE  },
    { MAIN_MENU_STATE,           EXIT_STATE                 },

    /* Game setup path */
    { GAME_MODE_SELECTION_STATE, GAME_SETUP_STATE           },
    { GAME_MODE_SELECTION_STATE, MAIN_MENU_STATE            },
    { GAME_SETUP_STATE,          GAMEPLAY_STATE             },
    { GAME_SETUP_STATE,          GAME_MODE_SELECTION_STATE  },

    /* Gameplay -> termination */
    { GAMEPLAY_STATE,            GAME_TERMINATION_STATE     },

    /* Termination -> end-game menu */
    { GAME_TERMINATION_STATE,    END_GAME_MENU_STATE        },

    /* End-game menu branches */
    { END_GAME_MENU_STATE,       GAME_MODE_SELECTION_STATE  },
    { END_GAME_MENU_STATE,       MAIN_MENU_STATE            },
    { END_GAME_MENU_STATE,       EXIT_STATE                 },

    /* EXIT_STATE */
    { MAIN_MENU_STATE,           EXIT_STATE                 },
    { GAME_MODE_SELECTION_STATE, EXIT_STATE                 },
    { GAME_SETUP_STATE,          EXIT_STATE                 },
    { GAMEPLAY_STATE,            EXIT_STATE                 },
    { GAME_TERMINATION_STATE,    EXIT_STATE                 },
};

static const int VALID_COUNT = (int)(sizeof(VALID) / sizeof(VALID[0]));

static int transitionIsAllowed(SystemState from, SystemState to) {
    if (from == to) return 1;
    for (int i = 0; i < VALID_COUNT; ++i) {
        if (VALID[i].from == from && VALID[i].to == to) return 1;
    }
    return 0;
}

void initFSM(void) {
    g_systemState = INIT_STATE;
}

SystemState getSystemState(void) {
    return g_systemState;
}

int transitionState(GameState *state, SystemState newState) {
    (void)state;
    if (!transitionIsAllowed(g_systemState, newState)) {
        return 1;
    }
    g_systemState = newState;
    return 0;
}

static int raiseError(EventQueue *outQueue, ErrorCode code) {
    if (!outQueue) return 0;
    Event err = createErrorEvent(code);
    return enqueueEvent(outQueue, err, queueForEvent(EVENT_ERROR));
}

static int handleInit(GameState *state, Event e, EventQueue *out) {
    (void)e; (void)out;
    return transitionState(state, MAIN_MENU_STATE);
}

static int handleMainMenu(GameState *state, Event e, EventQueue *out) {
    (void)out;
    switch (e.type) {
        case EVENT_NEW_GAME:
            return transitionState(state, GAME_MODE_SELECTION_STATE);
        case EVENT_EXIT_PROGRAM:
            return transitionState(state, EXIT_STATE);
        default:
            return 0;
    }
}

static int handleGameModeSelection(GameState *state, Event e, EventQueue *out) {
    (void)out;
    switch (e.type) {
        case EVENT_NEW_GAME:  /* mode chosen -> proceed to setup */
            return transitionState(state, GAME_SETUP_STATE);
        case EVENT_BACK:
            return transitionState(state, MAIN_MENU_STATE);
        case EVENT_EXIT_PROGRAM:
            return transitionState(state, EXIT_STATE);
        default:
            return 0;
    }
}

static int handleGameSetup(GameState *state, Event e, EventQueue *out) {
    (void)out;
    switch (e.type) {
        case EVENT_NEW_GAME:  /* setup confirmed -> start gameplay */
            logGameStart(state);
            return transitionState(state, GAMEPLAY_STATE);
        case EVENT_BACK:
            return transitionState(state, GAME_MODE_SELECTION_STATE);
        case EVENT_EXIT_PROGRAM:
            return transitionState(state, EXIT_STATE);
        default:
            return 0;
    }
}

static int handleGameplay(GameState *state, Event e, EventQueue *out) {
    int rc = 0;

    switch (e.type) {
        case EVENT_MOVE_INPUT:
            rc = runInputMovePipeline(state, e.data.command, out);
            if (rc == MOVE_PIPELINE_ERR_APPLY_FAILED) {
                return raiseError(out, ERR_FATAL_STATE_CORRUPT) ? 1 : 0;
            }
            break;

        case EVENT_AI_MOVE:
            rc = runAIMovePipeline(state, e.data.move, out);
            if (rc == MOVE_PIPELINE_ERR_APPLY_FAILED) {
                return raiseError(out, ERR_FATAL_STATE_CORRUPT) ? 1 : 0;
            }
            break;

        case EVENT_UNDO:
            rc = runUndoPipeline(state, out);
            (void)rc;
            break;

        case EVENT_TIMER_EXPIRED:
            /* Current player is out of time -> game ends. */
            state->gameOver = 1;
            state->result   = (state->currentTurn == WHITE)
                                ? RESULT_BLACK_WIN
                                : RESULT_WHITE_WIN;
            return transitionState(state, GAME_TERMINATION_STATE);

        case EVENT_LEAVE_GAME:
            state->result = RESULT_TERMINATED;
            return transitionState(state, GAME_TERMINATION_STATE);

        case EVENT_EXIT_PROGRAM:
            return transitionState(state, EXIT_STATE);

        default:
            break;
    }

    if (state->gameOver) {
        return transitionState(state, GAME_TERMINATION_STATE);
    }
    return 0;
}

static int handleGameTermination(GameState *state, Event e, EventQueue *out) {
    (void)e; (void)out;
    logGameEnd(state);
    return transitionState(state, END_GAME_MENU_STATE);
}

static int handleEndGameMenu(GameState *state, Event e, EventQueue *out) {
    (void)out;
    switch (e.type) {
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

static int handleFatalError(GameState *state, Event e, EventQueue *out) {
    (void)e; (void)out;
    g_systemState = EXIT_STATE; 
    (void)state;
    return 0;
}

int processEvent(GameState *state, Event event, EventQueue *outQueue) {
    if (!state) return 1;
    if (event.type == EVENT_FATAL_ERROR) {
        return handleFatalError(state, event, outQueue);
    }

    switch (g_systemState) {
        case INIT_STATE:                return handleInit              (state, event, outQueue);
        case MAIN_MENU_STATE:           return handleMainMenu          (state, event, outQueue);
        case GAME_MODE_SELECTION_STATE: return handleGameModeSelection (state, event, outQueue);
        case GAME_SETUP_STATE:          return handleGameSetup         (state, event, outQueue);
        case GAMEPLAY_STATE:            return handleGameplay          (state, event, outQueue);
        case GAME_TERMINATION_STATE:    return handleGameTermination   (state, event, outQueue);
        case END_GAME_MENU_STATE:       return handleEndGameMenu       (state, event, outQueue);
        case EXIT_STATE:                return 0; /* terminal */
        default:                        return 1;
    }
}

/* Debug */

const char *systemStateName(SystemState s) {
    switch (s) {
        case INIT_STATE:                return "INIT_STATE";
        case MAIN_MENU_STATE:           return "MAIN_MENU_STATE";
        case GAME_MODE_SELECTION_STATE: return "GAME_MODE_SELECTION_STATE";
        case GAME_SETUP_STATE:          return "GAME_SETUP_STATE";
        case GAMEPLAY_STATE:            return "GAMEPLAY_STATE";
        case GAME_TERMINATION_STATE:    return "GAME_TERMINATION_STATE";
        case END_GAME_MENU_STATE:       return "END_GAME_MENU_STATE";
        case EXIT_STATE:                return "EXIT_STATE";
        default:                        return "UNKNOWN";
    }
}