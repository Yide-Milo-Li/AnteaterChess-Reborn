#include "system/controller.h"

#include <stddef.h>

#include "system/event.h"
#include "system/event_queue.h"
#include "system/fsm.h"
#include "system/system_state.h"
#include "turn/turn_timer.h"

/*
 * Alignment assumptions for future extensions:
 * - controller.h is the truth source for the public loop entrypoints.
 * - This file owns queue polling and priority draining, not gameplay rule decisions.
 * - FSM remains the authority on state transitions while controller orchestrates event flow.
 */

/* Map one event type to the queue priority class expected by the controller. */
QueueType queueForEvent(EventType type) {
    switch (type) {
        /* Control events */
        case EVENT_LEAVE_GAME:
        case EVENT_BACK:
        case EVENT_NEW_GAME:
        case EVENT_EXIT_PROGRAM:
        case EVENT_ERROR:
        case EVENT_FATAL_ERROR:
            return QUEUE_CONTROL;

        /* System events */
        case EVENT_TIMER_EXPIRED:
            return QUEUE_SYSTEM;

        /* Gameplay events */
        case EVENT_MOVE_INPUT:
        case EVENT_AI_MOVE:
        case EVENT_UNDO:
        case EVENT_HINT:
            return QUEUE_GAMEPLAY;

        default:
            return QUEUE_GAMEPLAY;
    }
}

/* Poll one source once and enqueue any non-empty event it produces. */
static void pollAndEnqueue(EventPollerFn poller,
                           GameState *state,
                           EventQueue *queue)
{
    if (!poller) return;
    Event e;
    if (poller(state, &e) == 1) {
        if (e.type != EVENT_NONE) {
            enqueueEvent(queue, e, queueForEvent(e.type));
        }
    }
}

/* Single-tick body */
/* Execute one controller tick: poll sources, synthesize timer events, then drain queues. */
int tickGameLoop(GameState *state, EventQueue *queue, const EventSources *src) {

    if (getSystemState() == INIT_STATE) {
        Event none = createSystemEvent(EVENT_NONE);
        processEvent(state, none, queue);
    }

    if (src) {
        pollAndEnqueue(src->pollUI,     state, queue);
        pollAndEnqueue(src->pollAI,     state, queue);
        pollAndEnqueue(src->pollTimer,  state, queue);
        pollAndEnqueue(src->pollSystem, state, queue);
    }

    if (getSystemState() == GAMEPLAY_STATE) {
        updateTurnTimer(state);
        if (isTimeUp(state) == 1) {
            Event te = createSystemEvent(EVENT_TIMER_EXPIRED);
            enqueueEvent(queue, te, QUEUE_SYSTEM);
        }
    }

    const int MAX_PROCESS_PER_TICK = MAX_EVENTS * 3;
    int processed = 0;

    while (!isEventQueueEmpty(queue) && processed < MAX_PROCESS_PER_TICK) {
        Event ev;

        if (!isControlQueueEmpty(queue)) {
            ev = dequeueControlEvent(queue);
        } else if (!isSystemQueueEmpty(queue)) {
            ev = dequeueSystemEvent(queue);
        } else {
            ev = dequeueGameplayEvent(queue);
        }

        if (processEvent(state, ev, queue) != 0) {
            return (int)getSystemState();
        }
        ++processed;

        if (getSystemState() == EXIT_STATE) break;
    }

    return (int)getSystemState();
}

/* Main loop */

/* Run the controller loop until the FSM reaches EXIT_STATE. */
int runGameLoop(GameState *state, EventQueue *queue, const EventSources *src) {
    if (!state || !queue) return 1;

    while (getSystemState() != EXIT_STATE) {
        tickGameLoop(state, queue, src);
    }
    return 0;
}
