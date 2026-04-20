#include "system/event_queue.h"

#include <stddef.h>
#include <string.h>

/*
 * Alignment assumptions for future extensions:
 * - event_queue.h is the truth source for the public queue API and data layout.
 * - This file should only manage queue storage and ordering, never controller policy.
 * - Control/system/gameplay priority is enforced by the controller, not inside these primitives.
 */

/* Return an explicit EVENT_NONE sentinel for empty-dequeue paths. */
static Event none_event(void) {
    Event event;

    memset(&event, 0, sizeof(event));
    event.type = EVENT_NONE;
    return event;
}

/* Report whether one circular queue currently holds no events. */
static int single_queue_is_empty(const SingleEventQueue *queue) {
    return queue->front == queue->rear;
}

/* Report whether one circular queue has reached MAX_EVENTS capacity. */
static int single_queue_is_full(const SingleEventQueue *queue) {
    return (queue->rear - queue->front) >= MAX_EVENTS;
}

/* Append one event to a single queue while preserving FIFO order. */
static int single_queue_enqueue(SingleEventQueue *queue, Event event) {
    if (single_queue_is_full(queue)) {
        return 1;
    }

    queue->events[queue->rear % MAX_EVENTS] = event;
    queue->rear += 1;
    return 0;
}

/* Remove and return the oldest event from one queue. */
static Event single_queue_dequeue(SingleEventQueue *queue) {
    Event event;

    if (single_queue_is_empty(queue)) {
        return none_event();
    }

    event = queue->events[queue->front % MAX_EVENTS];
    queue->front += 1;

    /* Rebase counters after the queue becomes empty so long sessions do not
     * let the monotonic indices grow without bound. */
    if (queue->front == queue->rear) {
        queue->front = 0;
        queue->rear = 0;
    }

    return event;
}

/* Route one event into the requested public subqueue. */
int enqueueEvent(EventQueue *queue, Event event, QueueType type) {
    if (queue == NULL) {
        return 1;
    }

    switch (type) {
        case QUEUE_CONTROL:
            return single_queue_enqueue(&queue->controlQueue, event);
        case QUEUE_SYSTEM:
            return single_queue_enqueue(&queue->systemQueue, event);
        case QUEUE_GAMEPLAY:
            return single_queue_enqueue(&queue->gameplayQueue, event);
        default:
            return 1;
    }
}

/* Remove the next control-priority event or EVENT_NONE if unavailable. */
Event dequeueControlEvent(EventQueue *queue) {
    if (queue == NULL) {
        return none_event();
    }

    return single_queue_dequeue(&queue->controlQueue);
}

/* Remove the next system-priority event or EVENT_NONE if unavailable. */
Event dequeueSystemEvent(EventQueue *queue) {
    if (queue == NULL) {
        return none_event();
    }

    return single_queue_dequeue(&queue->systemQueue);
}

/* Remove the next gameplay-priority event or EVENT_NONE if unavailable. */
Event dequeueGameplayEvent(EventQueue *queue) {
    if (queue == NULL) {
        return none_event();
    }

    return single_queue_dequeue(&queue->gameplayQueue);
}

/* Report whether the control queue currently has no pending events. */
int isControlQueueEmpty(const EventQueue *queue) {
    if (queue == NULL) {
        return 1;
    }

    return single_queue_is_empty(&queue->controlQueue);
}

/* Report whether the system queue currently has no pending events. */
int isSystemQueueEmpty(const EventQueue *queue) {
    if (queue == NULL) {
        return 1;
    }

    return single_queue_is_empty(&queue->systemQueue);
}

/* Report whether the gameplay queue currently has no pending events. */
int isGameplayQueueEmpty(const EventQueue *queue) {
    if (queue == NULL) {
        return 1;
    }

    return single_queue_is_empty(&queue->gameplayQueue);
}
