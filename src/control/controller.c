#include "system/controller.h"

#include <stddef.h>

#include "system/event.h"
#include "system/fsm.h"
#include "turn/turn_timer.h"

/*
 * Alignment assumptions for future extensions:
 * - controller.h is the truth source for the public loop entrypoint.
 * - This file owns queue draining and priority ordering, not gameplay rule decisions.
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

/* Remove the highest-priority available event from the local controller queue. */
static Event dequeue_next_event(EventQueue *queue) {
    if (!isControlQueueEmpty(queue)) {
        return dequeueControlEvent(queue);
    }

    if (!isSystemQueueEmpty(queue)) {
        return dequeueSystemEvent(queue);
    }

    return dequeueGameplayEvent(queue);
}

/* Enqueue controller-synthesized work such as boot and termination handshakes. */
static int seed_internal_events(GameState *state, EventQueue *queue) {
    Event event;

    if (state == NULL || queue == NULL) {
        return 1;
    }

    if (state->systemState == INIT_STATE || state->systemState == GAME_TERMINATION_STATE) {
        event = createSystemEvent(EVENT_NONE);
        return enqueueEvent(queue, event, queue_type_for_event(event.type));
    }

    if (state->systemState == GAMEPLAY_STATE) {
        if (updateTurnTimer(state) != 0) {
            return 1;
        }

        if (isTimeUp(state) == 1) {
            event = createSystemEvent(EVENT_TIMER_EXPIRED);
            return enqueueEvent(queue, event, queue_type_for_event(event.type));
        }
    }

    return 0;
}

/* Drain the available event work until the queue empties or a terminal condition is reached. */
int runGameLoop(GameState *state) {
    EventQueue queue = {0};

    if (state == NULL) {
        return 1;
    }

    while (state->systemState != EXIT_STATE) {
        if (all_queues_empty(&queue)) {
            if (seed_internal_events(state, &queue) != 0) {
                return 1;
            }

            if (all_queues_empty(&queue)) {
                break;
            }
        }

        if (processEvent(state, dequeue_next_event(&queue)) != 0) {
            return 1;
        }

        if (state->gameOver && state->systemState == END_GAME_MENU_STATE) {
            break;
        }
    }

    return 0;
}
