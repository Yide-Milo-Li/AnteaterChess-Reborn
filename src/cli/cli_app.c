#include "cli/cli_app.h"

#include <stdio.h>
#include <stddef.h>

#include "cli/cli_feedback.h"
#include "cli/cli_gameplay.h"
#include "cli/cli_menu.h"
#include "cli/cli_renderer.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "system/event.h"
#include "system/event_queue.h"
#include "system/fsm.h"
#include "turn/turn_timer.h"

/*
 * Alignment assumptions for future extensions:
 * - CLI headers are the truth source for the text-front-end contract in this file.
 * - This file orchestrates a standalone CLI app loop and must not define GUI behavior.
 * - FSM and gameplay modules remain the source of truth for state transitions and rules.
 */

#define ANSI_RESET  "\x1b[0m"
#define ANSI_ACCENT "\x1b[1;36m"

/* Map one event type to the queue used by the standalone CLI app loop. */
static QueueType queue_type_for_event(EventType type) {
    switch (type) {
        case EVENT_LEAVE_GAME:
        case EVENT_BACK:
        case EVENT_NEW_GAME:
        case EVENT_EXIT_PROGRAM:
        case EVENT_ERROR:
        case EVENT_FATAL_ERROR:
            return QUEUE_CONTROL;
        case EVENT_TIMER_EXPIRED:
            return QUEUE_SYSTEM;
        case EVENT_MOVE_INPUT:
        case EVENT_AI_MOVE:
        case EVENT_UNDO:
        case EVENT_HINT:
        case EVENT_NONE:
        default:
            return QUEUE_GAMEPLAY;
    }
}

/* Check whether the local CLI event queue currently has pending work. */
static int all_queues_empty(const EventQueue *queue) {
    return isControlQueueEmpty(queue)
        && isSystemQueueEmpty(queue)
        && isGameplayQueueEmpty(queue);
}

/* Remove one event using the same priority order the system controller expects. */
static Event dequeue_next_event(EventQueue *queue) {
    if (!isControlQueueEmpty(queue)) {
        return dequeueControlEvent(queue);
    }

    if (!isSystemQueueEmpty(queue)) {
        return dequeueSystemEvent(queue);
    }

    return dequeueGameplayEvent(queue);
}

/* Enqueue one event for later prioritized dispatch. */
static int enqueue_cli_event(EventQueue *queue, Event event) {
    return enqueueEvent(queue, event, queue_type_for_event(event.type));
}

/* Print one lightweight gameplay page header before the status panel and board. */
static void print_gameplay_page_header(void) {
    printf("\n%s══════════════════ Anteater Chess CLI ══════════════════%s\n",
        ANSI_ACCENT, ANSI_RESET);
    printf("%sGameplay View%s\n", ANSI_ACCENT, ANSI_RESET);
}

/* Show the disabled AI-mode message without changing FSM state. */
static void handle_disabled_mode_choice(void) {
    (void)cliShowDisabledFeatureMessage("AI game modes");
}

/* Collect the next main-menu event for the CLI frontend. */
static int collect_main_menu_event(EventQueue *queue) {
    int selection;

    if (cliGetMainMenuSelection(&selection) != 0) {
        return 1;
    }

    if (selection == 1) {
        return enqueue_cli_event(queue, createSystemEvent(EVENT_NEW_GAME));
    }

    return enqueue_cli_event(queue, createSystemEvent(EVENT_EXIT_PROGRAM));
}

/* Collect the next game-mode event for the CLI frontend. */
static int collect_mode_selection_event(EventQueue *queue) {
    int selection;

    if (cliGetGameModeSelection(&selection) != 0) {
        return 1;
    }

    switch (selection) {
        case 1:
            return enqueue_cli_event(queue, createSystemEvent(EVENT_NEW_GAME));
        case 2:
        case 3:
            handle_disabled_mode_choice();
            return 0;
        case 4:
            return enqueue_cli_event(queue, createSystemEvent(EVENT_BACK));
        case 5:
            return enqueue_cli_event(queue, createSystemEvent(EVENT_EXIT_PROGRAM));
        default:
            return 1;
    }
}

/* Collect setup fields, reinitialize the game state, and continue into gameplay. */
static int collect_setup_event(GameState *state, EventQueue *queue) {
    GameConfig config;

    if (state == NULL) {
        return 1;
    }

    if (cliGetGameSetupConfig(&config) != 0) {
        return 1;
    }

    initGameState(state, &config);
    state->systemState = GAME_SETUP_STATE;
    return enqueue_cli_event(queue, createSystemEvent(EVENT_NEW_GAME));
}

/* Collect one gameplay action and map it to the next CLI event if any. */
static int collect_gameplay_event(GameState *state, EventQueue *queue) {
    int action;
    Command command;

    if (state == NULL) {
        return 1;
    }

    if (state->config.timerEnabled) {
        if (updateTurnTimer(state) != 0) {
            return 1;
        }
        if (isTimeUp(state) == 1) {
            return enqueue_cli_event(queue, createSystemEvent(EVENT_TIMER_EXPIRED));
        }
    }

    print_gameplay_page_header();
    if (cliDisplayTurn(state->currentTurn) != 0
        || cliDisplayGameStatus(state) != 0
        || cliRenderBoard(state) != 0) {
        return 1;
    }

    if (cliGetGameplayAction(&action) != 0) {
        return 1;
    }

    if (state->config.timerEnabled) {
        if (updateTurnTimer(state) != 0) {
            return 1;
        }
        if (isTimeUp(state) == 1) {
            return enqueue_cli_event(queue, createSystemEvent(EVENT_TIMER_EXPIRED));
        }
    }

    switch (action) {
        case 1:
            if (cliGetMoveCommand(&command) != 0) {
                cliShowErrorMessage(ERR_INVALID_INPUT);
                return 0;
            }
            return enqueue_cli_event(queue, createMoveInputEvent(command));
        case 2:
            if (state->moveHistory.count <= 0) {
                cliShowErrorMessage(ERR_UNDO_UNAVAILABLE);
                return 0;
            }
            return enqueue_cli_event(queue, createUndoEvent());
        case 3:
            return enqueue_cli_event(queue, createSystemEvent(EVENT_LEAVE_GAME));
        case 4:
            return enqueue_cli_event(queue, createSystemEvent(EVENT_EXIT_PROGRAM));
        case 5:
            return cliShowMoveFormatHint();
        default:
            return 1;
    }
}

/* Collect the next end-game menu event for the CLI frontend. */
static int collect_endgame_event(const GameState *state, EventQueue *queue) {
    int selection;

    if (cliShowEndGameMenu(state, &selection) != 0) {
        return 1;
    }

    switch (selection) {
        case 1:
            return enqueue_cli_event(queue, createSystemEvent(EVENT_NEW_GAME));
        case 2:
            return enqueue_cli_event(queue, createSystemEvent(EVENT_BACK));
        case 3:
            return enqueue_cli_event(queue, createSystemEvent(EVENT_EXIT_PROGRAM));
        default:
            return 1;
    }
}

/* Synthesize internal events that do not come directly from CLI input. */
static int collect_internal_event(GameState *state, EventQueue *queue) {
    if (state == NULL) {
        return 1;
    }

    if (state->systemState == INIT_STATE || state->systemState == GAME_TERMINATION_STATE) {
        return enqueue_cli_event(queue, createSystemEvent(EVENT_NONE));
    }

    return 0;
}

/* Collect the next event for the standalone CLI app based on the active system state. */
static int collect_next_cli_event(GameState *state, EventQueue *queue) {
    if (state == NULL || queue == NULL) {
        return 1;
    }

    if (!all_queues_empty(queue)) {
        return 0;
    }

    if (state->systemState == INIT_STATE || state->systemState == GAME_TERMINATION_STATE) {
        return collect_internal_event(state, queue);
    }

    switch (state->systemState) {
        case MAIN_MENU_STATE:
            return collect_main_menu_event(queue);
        case GAME_MODE_SELECTION_STATE:
            return collect_mode_selection_event(queue);
        case GAME_SETUP_STATE:
            return collect_setup_event(state, queue);
        case GAMEPLAY_STATE:
            return collect_gameplay_event(state, queue);
        case END_GAME_MENU_STATE:
            return collect_endgame_event(state, queue);
        case EXIT_STATE:
            return 0;
        default:
            return 1;
    }
}

/* Translate common FSM failures into user-visible CLI feedback. */
static void report_processing_error(const GameState *state, Event event) {
    if (event.type == EVENT_MOVE_INPUT) {
        cliShowErrorMessage(ERR_ILLEGAL_MOVE);
        return;
    }

    if (event.type == EVENT_UNDO) {
        cliShowErrorMessage(ERR_UNDO_UNAVAILABLE);
        return;
    }

    if (event.type == EVENT_HINT) {
        cliShowErrorMessage(ERR_HINT_UNAVAILABLE);
        return;
    }

    if (state != NULL && state->config.timerEnabled && state->systemState == GAMEPLAY_STATE && isTimeUp(state) == 1) {
        cliShowErrorMessage(ERR_TIME_UP);
        return;
    }

    cliShowErrorMessage(ERR_FATAL);
}

/* Run the standalone CLI application loop until the FSM reaches EXIT_STATE. */
int runCliApp(void) {
    GameConfig config;
    GameState state;
    EventQueue queue = {0};

    initDefaultGameConfig(&config);
    initGameState(&state, &config);

    while (state.systemState != EXIT_STATE) {
        Event event;

        if (collect_next_cli_event(&state, &queue) != 0) {
            cliShowErrorMessage(ERR_FATAL);
            return 1;
        }

        if (all_queues_empty(&queue)) {
            continue;
        }

        event = dequeue_next_event(&queue);
        if (processEvent(&state, event) != 0) {
            report_processing_error(&state, event);
        }
    }

    return 0;
}
