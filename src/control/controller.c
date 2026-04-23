#include "system/controller.h"

#include <stddef.h>

#include "ai/ai.h"
#include "system/fsm.h"
#include "turn/turn_timer.h"

/*
 * Alignment assumptions for future extensions:
 * - controller.h is the preferred public integration layer for future UI work.
 * - This file owns runtime orchestration concerns such as queue draining,
 *   priority ordering, and controller-synthesized events.
 * - Gameplay rule decisions and event semantics still belong to the FSM.
 * - Queue helpers that are not declared in headers remain private to the controller.
 */

/* Map one public event type to the private queue priority used by the controller. */
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

/* Report whether the composite queue has any pending work left to drain. */
static int all_queues_empty(const EventQueue *queue) {
    return isControlQueueEmpty(queue)
        && isSystemQueueEmpty(queue)
        && isGameplayQueueEmpty(queue);
}

/* Remove the highest-priority available event from the controller queue. */
static Event dequeue_next_event(EventQueue *queue) {
    if (!isControlQueueEmpty(queue)) {
        return dequeueControlEvent(queue);
    }

    if (!isSystemQueueEmpty(queue)) {
        return dequeueSystemEvent(queue);
    }

    return dequeueGameplayEvent(queue);
}

/* Report whether the current side to move is AI-controlled. */
static int current_turn_is_ai(const GameState *state) {
    if (state == NULL || state->currentTurn < WHITE || state->currentTurn > BLACK) {
        return 0;
    }

    return state->players[state->currentTurn].type == AI;
}

/* Report whether an externally submitted gameplay event should still honor the
 * active human turn timer before it enters the queue. */
static int is_human_gameplay_input(EventType type) {
    switch (type) {
        case EVENT_MOVE_INPUT:
        case EVENT_UNDO:
        case EVENT_LEAVE_GAME:
        case EVENT_EXIT_PROGRAM:
        case EVENT_HINT:
            return 1;
        default:
            return 0;
    }
}

/* Enqueue one event using the controller's priority policy. */
static int enqueue_controller_event(Controller *controller, Event event) {
    if (controller == NULL) {
        return 1;
    }

    return enqueueEvent(&controller->queue, event, queue_type_for_event(event.type));
}

/* Lifecycle advancement is the only controller path that synthesizes
 * EVENT_NONE. It keeps INIT and termination compatibility out of frontend code. */
static int seed_lifecycle_event(Controller *controller) {
    GameState *state;

    if (controller == NULL) {
        return 1;
    }

    state = &controller->state;
    if (state->systemState != INIT_STATE && state->systemState != GAME_TERMINATION_STATE) {
        return 0;
    }

    return enqueue_controller_event(controller, createSystemEvent(EVENT_NONE));
}

/* Timer checks belong to controller orchestration. The FSM only decides what
 * EVENT_TIMER_EXPIRED means once this event reaches gameplay state. */
static int seed_timer_expiry_if_needed(Controller *controller) {
    GameState *state;

    if (controller == NULL) {
        return 1;
    }

    state = &controller->state;
    if (state->systemState != GAMEPLAY_STATE || !state->config.timerEnabled) {
        return 0;
    }

    if (updateTurnTimer(state) != 0) {
        return 1;
    }

    if (isTimeUp(state) == 1) {
        return enqueue_controller_event(controller, createSystemEvent(EVENT_TIMER_EXPIRED));
    }

    return 0;
}

/* AI scheduling is controller-owned background work. The selected move is still
 * applied by the FSM after it receives EVENT_AI_MOVE. */
static int seed_ai_turn_if_needed(Controller *controller) {
    GameState *state;
    Move move;

    if (controller == NULL) {
        return 1;
    }

    state = &controller->state;
    if (state->systemState != GAMEPLAY_STATE || !current_turn_is_ai(state)) {
        return 0;
    }

    if (generateAIMove(state, &move) != 0) {
        return 1;
    }

    if (seed_timer_expiry_if_needed(controller) != 0) {
        return 1;
    }

    if (!all_queues_empty(&controller->queue)) {
        return 0;
    }

    return enqueue_controller_event(controller, createAIMoveEvent(move));
}

/* Background work only covers gameplay timers and AI turns. It never performs
 * lifecycle EVENT_NONE advancement. */
static int seed_background_events(Controller *controller) {
    if (controller == NULL) {
        return 1;
    }

    if (controller->state.systemState != GAMEPLAY_STATE) {
        return 0;
    }

    if (seed_timer_expiry_if_needed(controller) != 0) {
        return 1;
    }

    if (!all_queues_empty(&controller->queue)) {
        return 0;
    }

    return seed_ai_turn_if_needed(controller);
}

/* Seed exactly one category of controller-owned work when external queues are
 * empty: lifecycle first, then gameplay background work. */
static int seed_controller_owned_events(Controller *controller) {
    if (controller == NULL) {
        return 1;
    }

    if (seed_lifecycle_event(controller) != 0) {
        return 1;
    }

    if (!all_queues_empty(&controller->queue)) {
        return 0;
    }

    return seed_background_events(controller);
}

/* Dispatch one chosen event into the FSM. Queue selection and event synthesis
 * have already happened before this point. */
static int dispatch_controller_event(Controller *controller, Event event) {
    if (controller == NULL) {
        return 1;
    }

    return processEvent(&controller->state, event);
}

/* Bring the controller to a user-facing state before accepting a frontend
 * intent, so lifecycle compatibility events do not consume that intent. */
static int advance_lifecycle_before_external_request(Controller *controller) {
    if (controller == NULL) {
        return 1;
    }

    if (controller->state.systemState == INIT_STATE
        || controller->state.systemState == GAME_TERMINATION_STATE) {
        return controllerRunUntilIdle(controller);
    }

    return 0;
}

/* Apply one external request by routing it through the public queue policy and
 * draining follow-up controller work before returning to the caller. */
static int apply_external_request(Controller *controller, Event event) {
    if (advance_lifecycle_before_external_request(controller) != 0) {
        return 1;
    }

    if (controllerEnqueueEvent(controller, event) != 0) {
        return 1;
    }

    return controllerRunUntilIdle(controller);
}

/* Initialize one controller value with a fresh game state and an empty queue. */
void initController(Controller *controller, const GameConfig *config) {
    if (controller == NULL) {
        return;
    }

    initGameState(&controller->state, config);
    controller->queue = (EventQueue){0};
}

/* Return the read-only runtime state owned by one controller. */
const GameState *controllerGetState(const Controller *controller) {
    if (controller == NULL) {
        return NULL;
    }

    return &controller->state;
}

/* Route one externally supplied event through the controller queue policy. */
int controllerEnqueueEvent(Controller *controller, Event event) {
    GameState *state;

    if (controller == NULL) {
        return 1;
    }

    state = &controller->state;
    if (state->systemState == GAMEPLAY_STATE
        && state->config.timerEnabled
        && !current_turn_is_ai(state)
        && is_human_gameplay_input(event.type)) {
        if (updateTurnTimer(state) != 0) {
            return 1;
        }

        if (isTimeUp(state) == 1) {
            return enqueue_controller_event(controller, createSystemEvent(EVENT_TIMER_EXPIRED));
        }
    }

    return enqueue_controller_event(controller, event);
}

/* Process at most one queued or controller-synthesized event. EVENT_NONE in
 * processedEvent means either one compatibility no-op event was processed or
 * the controller was already idle. */
int controllerTick(Controller *controller, Event *processedEvent) {
    Event event;

    if (processedEvent != NULL) {
        *processedEvent = createSystemEvent(EVENT_NONE);
    }

    if (controller == NULL) {
        return 1;
    }

    if (all_queues_empty(&controller->queue)) {
        if (seed_controller_owned_events(controller) != 0) {
            return 1;
        }

        if (all_queues_empty(&controller->queue)) {
            return 0;
        }
    }

    event = dequeue_next_event(&controller->queue);
    if (processedEvent != NULL) {
        *processedEvent = event;
    }

    return dispatch_controller_event(controller, event);
}

/* Drain queued and controller-synthesized work until the controller becomes
 * idle. This helper never blocks for UI input. */
int controllerRunUntilIdle(Controller *controller) {
    Event processedEvent;

    if (controller == NULL) {
        return 1;
    }

    for (;;) {
        SystemState previousState;
        int hadPendingEvents;

        previousState = controller->state.systemState;
        hadPendingEvents = !all_queues_empty(&controller->queue);
        if (controllerTick(controller, &processedEvent) != 0) {
            return 1;
        }

        if (processedEvent.type != EVENT_NONE) {
            continue;
        }

        if (previousState != controller->state.systemState || hadPendingEvents) {
            continue;
        }

        if (!all_queues_empty(&controller->queue)) {
            continue;
        }

        break;
    }

    return 0;
}

/* Process one explicit system event and verify that the FSM reached the
 * expected menu state. This keeps the configured-start path readable while
 * preserving the public menu graph. */
static int drive_system_event_to_state(Controller *controller, EventType type, SystemState expectedState) {
    Event processedEvent;

    if (controller == NULL) {
        return 1;
    }

    if (controllerEnqueueEvent(controller, createSystemEvent(type)) != 0
        || controllerTick(controller, &processedEvent) != 0) {
        return 1;
    }

    if (processedEvent.type != type) {
        return 1;
    }

    return (controller->state.systemState == expectedState) ? 0 : 1;
}

/* Drive boot compatibility into the first visible menu. */
static int drive_boot_to_main_menu(Controller *controller) {
    Event processedEvent;

    if (controller == NULL) {
        return 1;
    }

    if (controllerTick(controller, &processedEvent) != 0) {
        return 1;
    }

    if (processedEvent.type != EVENT_NONE) {
        return 1;
    }

    return (controller->state.systemState == MAIN_MENU_STATE) ? 0 : 1;
}

/* Drive the public menu path up to the setup screen without starting gameplay. */
static int drive_to_configured_game_setup(Controller *controller) {
    if (drive_boot_to_main_menu(controller) != 0) {
        return 1;
    }

    if (drive_system_event_to_state(controller, EVENT_NEW_GAME, GAME_MODE_SELECTION_STATE) != 0) {
        return 1;
    }

    return drive_system_event_to_state(controller, EVENT_NEW_GAME, GAME_SETUP_STATE);
}

/* Start gameplay from setup, leaving the first gameplay background tick to the
 * caller so AI openings can still be observed as processed events. */
static int drive_setup_to_gameplay(Controller *controller) {
    return drive_system_event_to_state(controller, EVENT_NEW_GAME, GAMEPLAY_STATE);
}

/* Rebuild a controller from configuration and drive only the setup-to-gameplay
 * bootstrap path, leaving the first gameplay tick to the caller. */
int controllerStartConfiguredGame(Controller *controller, const GameConfig *config) {
    if (controller == NULL) {
        return 1;
    }

    initController(controller, config);

    if (drive_to_configured_game_setup(controller) != 0) {
        return 1;
    }

    return drive_setup_to_gameplay(controller);
}

/* Advance one controller along the public "new game" flow until it next goes
 * idle in a UI-facing state. */
int controllerRequestNewGame(Controller *controller) {
    return apply_external_request(controller, createSystemEvent(EVENT_NEW_GAME));
}

/* Advance one controller along the public "back" flow until it next goes
 * idle in a UI-facing state. */
int controllerRequestBack(Controller *controller) {
    return apply_external_request(controller, createSystemEvent(EVENT_BACK));
}

/* Advance one controller along the public "exit" flow until it reaches the
 * next stable state. */
int controllerRequestExit(Controller *controller) {
    return apply_external_request(controller, createSystemEvent(EVENT_EXIT_PROGRAM));
}

/* Submit one already parsed move command and drain controller-owned follow-up
 * work such as AI replies, timers, or termination transitions. */
int controllerSubmitMove(Controller *controller, Command command) {
    if (command.type != CMD_MOVE) {
        return 1;
    }

    if (controller == NULL || advance_lifecycle_before_external_request(controller) != 0) {
        return 1;
    }

    if (controller->state.systemState != GAMEPLAY_STATE) {
        return 1;
    }

    if (controllerEnqueueEvent(controller, createMoveInputEvent(command)) != 0) {
        return 1;
    }

    return controllerRunUntilIdle(controller);
}

/* Request one undo and drain controller-owned follow-up work before the GUI
 * reads back state again. */
int controllerRequestUndo(Controller *controller) {
    if (controller == NULL || advance_lifecycle_before_external_request(controller) != 0) {
        return 1;
    }

    if (controller->state.systemState != GAMEPLAY_STATE) {
        return 1;
    }

    if (controllerEnqueueEvent(controller, createUndoEvent()) != 0) {
        return 1;
    }

    return controllerRunUntilIdle(controller);
}

/* Request user-triggered gameplay termination and drain the transition into
 * the next stable menu state. */
int controllerRequestLeaveGame(Controller *controller) {
    if (controller == NULL || advance_lifecycle_before_external_request(controller) != 0) {
        return 1;
    }

    if (controller->state.systemState != GAMEPLAY_STATE) {
        return 1;
    }

    if (controllerEnqueueEvent(controller, createSystemEvent(EVENT_LEAVE_GAME)) != 0) {
        return 1;
    }

    return controllerRunUntilIdle(controller);
}

/* Return one hint move for the current gameplay position without mutating the
 * controller-owned state. */
int controllerGetHint(const Controller *controller, Move *move) {
    if (controller == NULL || move == NULL) {
        return 1;
    }

    if (controller->state.systemState != GAMEPLAY_STATE) {
        return 1;
    }

    return generateHintMove(&controller->state, move);
}

/* Drive one legacy public runtime loop by copying state into a temporary
 * controller, draining controller-owned work, then copying state back out. */
int runGameLoop(GameState *state) {
    Controller controller;

    if (state == NULL) {
        return 1;
    }

    controller.state = *state;
    controller.queue = (EventQueue){0};
    if (controllerRunUntilIdle(&controller) != 0) {
        return 1;
    }

    *state = controller.state;
    return 0;
}
