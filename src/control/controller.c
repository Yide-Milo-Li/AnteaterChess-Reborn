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

/* Enqueue controller-synthesized work such as boot, termination, timers, and
 * AI turns. */
static int seed_internal_events(Controller *controller) {
    GameState *state;
    Move move;

    if (controller == NULL) {
        return 1;
    }

    state = &controller->state;
    if (state->systemState == INIT_STATE || state->systemState == GAME_TERMINATION_STATE) {
        return enqueue_controller_event(controller, createSystemEvent(EVENT_NONE));
    }

    if (state->systemState != GAMEPLAY_STATE) {
        return 0;
    }

    if (state->config.timerEnabled) {
        if (updateTurnTimer(state) != 0) {
            return 1;
        }

        if (isTimeUp(state) == 1) {
            return enqueue_controller_event(controller, createSystemEvent(EVENT_TIMER_EXPIRED));
        }
    }

    if (!current_turn_is_ai(state)) {
        return 0;
    }

    if (generateAIMove(state, &move) != 0) {
        return 1;
    }

    if (state->config.timerEnabled) {
        if (updateTurnTimer(state) != 0) {
            return 1;
        }

        if (isTimeUp(state) == 1) {
            return enqueue_controller_event(controller, createSystemEvent(EVENT_TIMER_EXPIRED));
        }
    }

    return enqueue_controller_event(controller, createAIMoveEvent(move));
}

/* Settle one controller into the next stable user-facing state before an
 * external request runs, so INIT and termination handshakes do not swallow the
 * caller's intent event. */
static int settle_for_external_request(Controller *controller) {
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
    if (settle_for_external_request(controller) != 0) {
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
        if (seed_internal_events(controller) != 0) {
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

    return processEvent(&controller->state, event);
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

/* Rebuild a controller from configuration and drive only the setup-to-gameplay
 * bootstrap path, leaving the first gameplay tick to the caller. */
int controllerStartConfiguredGame(Controller *controller, const GameConfig *config) {
    Event processedEvent;

    if (controller == NULL) {
        return 1;
    }

    initController(controller, config);

    if (controllerTick(controller, &processedEvent) != 0
        || controller->state.systemState != MAIN_MENU_STATE) {
        return 1;
    }

    if (controllerEnqueueEvent(controller, createSystemEvent(EVENT_NEW_GAME)) != 0
        || controllerTick(controller, &processedEvent) != 0
        || controller->state.systemState != GAME_MODE_SELECTION_STATE) {
        return 1;
    }

    if (controllerEnqueueEvent(controller, createSystemEvent(EVENT_NEW_GAME)) != 0
        || controllerTick(controller, &processedEvent) != 0
        || controller->state.systemState != GAME_SETUP_STATE) {
        return 1;
    }

    if (controllerEnqueueEvent(controller, createSystemEvent(EVENT_NEW_GAME)) != 0
        || controllerTick(controller, &processedEvent) != 0
        || controller->state.systemState != GAMEPLAY_STATE) {
        return 1;
    }

    return 0;
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

    if (settle_for_external_request(controller) != 0 || controller == NULL) {
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
    if (settle_for_external_request(controller) != 0 || controller == NULL) {
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
    if (settle_for_external_request(controller) != 0 || controller == NULL) {
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
