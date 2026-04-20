#include "system/event_queue.h"

#include <stddef.h>
#include <string.h>

/*
 * Alignment assumptions for future extensions:
 * - event_queue.h is the truth source for the public queue API and data layout.
 * - This file should only manage queue storage and ordering, never controller policy.
 * - Control/system/gameplay priority is enforced by the controller, not inside these primitives.
 */

/* Single-queue primitives */
/* Reset one circular queue to its empty state. */
static void seqInit(SingleEventQueue *q) {
    q->front = 0;
    q->rear  = 0;
    q->count = 0;
}

/* Report whether one circular queue currently holds no events. */
static int seqIsEmpty(const SingleEventQueue *q) {
    return q->count == 0;
}

/* Report whether one circular queue has reached MAX_EVENTS capacity. */
static int seqIsFull(const SingleEventQueue *q) {
    return q->count >= MAX_EVENTS;
}

/* Append one event to a single circular queue if capacity remains. */
static int seqEnqueue(SingleEventQueue *q, Event e) {
    if (seqIsFull(q)) {
        return 1; /* queue full */
    }
    q->events[q->rear] = e;
    q->rear = (q->rear + 1) % MAX_EVENTS;
    q->count += 1;
    return 0;
}

/* Remove and return the oldest event from one circular queue. */
static Event seqDequeue(SingleEventQueue *q) {
    Event none;
    memset(&none, 0, sizeof none);
    none.type = EVENT_NONE;

    if (seqIsEmpty(q)) {
        return none;
    }
    Event e = q->events[q->front];
    q->front = (q->front + 1) % MAX_EVENTS;
    q->count -= 1;
    return e;
}

/* Public API */

/* Reset all three priority queues in one EventQueue container. */
void initEventQueue(EventQueue *q) {
    if (!q) return;
    seqInit(&q->controlQueue);
    seqInit(&q->systemQueue);
    seqInit(&q->gameplayQueue);
}

/* Route an event into the requested subqueue. */
int enqueueEvent(EventQueue *q, Event e, QueueType type) {
    if (!q) return 1;
    switch (type) {
        case QUEUE_CONTROL: return seqEnqueue(&q->controlQueue,  e);
        case QUEUE_SYSTEM: return seqEnqueue(&q->systemQueue,   e);
        case QUEUE_GAMEPLAY: return seqEnqueue(&q->gameplayQueue, e);
        default: return 1; /* invalid queue type */
    }
}

/* Remove the next control-priority event or EVENT_NONE if unavailable. */
Event dequeueControlEvent(EventQueue *q) {
    Event none;
    memset(&none, 0, sizeof none);
    none.type = EVENT_NONE;
    if (!q) return none;
    return seqDequeue(&q->controlQueue);
}

/* Remove the next system-priority event or EVENT_NONE if unavailable. */
Event dequeueSystemEvent(EventQueue *q) {
    Event none;
    memset(&none, 0, sizeof none);
    none.type = EVENT_NONE;
    if (!q) return none;
    return seqDequeue(&q->systemQueue);
}

/* Remove the next gameplay-priority event or EVENT_NONE if unavailable. */
Event dequeueGameplayEvent(EventQueue *q) {
    Event none;
    memset(&none, 0, sizeof none);
    none.type = EVENT_NONE;
    if (!q) return none;
    return seqDequeue(&q->gameplayQueue);
}

/* Report whether the control queue currently has no pending events. */
int isControlQueueEmpty(const EventQueue *q) {
    if (!q) return 1;
    return seqIsEmpty(&q->controlQueue);
}

/* Report whether the system queue currently has no pending events. */
int isSystemQueueEmpty(const EventQueue *q) {
    if (!q) return 1;
    return seqIsEmpty(&q->systemQueue);
}

/* Report whether the gameplay queue currently has no pending events. */
int isGameplayQueueEmpty(const EventQueue *q) {
    if (!q) return 1;
    return seqIsEmpty(&q->gameplayQueue);
}

/* Report whether all three queues are empty at the same time. */
int isEventQueueEmpty(const EventQueue *q) {
    if (!q) return 1;
    return seqIsEmpty(&q->controlQueue)
        && seqIsEmpty(&q->systemQueue)
        && seqIsEmpty(&q->gameplayQueue);
}
