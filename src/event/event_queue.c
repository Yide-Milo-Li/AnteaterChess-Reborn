#include "system/event_queue.h"

#include <stddef.h>
#include <string.h>

/* Single-queue primitives */
static void seqInit(SingleEventQueue *q) {
    q->front = 0;
    q->rear  = 0;
    q->count = 0;
}

static int seqIsEmpty(const SingleEventQueue *q) {
    return q->count == 0;
}

static int seqIsFull(const SingleEventQueue *q) {
    return q->count >= MAX_EVENTS;
}

static int seqEnqueue(SingleEventQueue *q, Event e) {
    if (seqIsFull(q)) {
        return 1; /* queue full */
    }
    q->events[q->rear] = e;
    q->rear = (q->rear + 1) % MAX_EVENTS;
    q->count += 1;
    return 0;
}

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

void initEventQueue(EventQueue *q) {
    if (!q) return;
    seqInit(&q->controlQueue);
    seqInit(&q->systemQueue);
    seqInit(&q->gameplayQueue);
}

int enqueueEvent(EventQueue *q, Event e, QueueType type) {
    if (!q) return 1;
    switch (type) {
        case QUEUE_CONTROL: return seqEnqueue(&q->controlQueue,  e);
        case QUEUE_SYSTEM: return seqEnqueue(&q->systemQueue,   e);
        case QUEUE_GAMEPLAY: return seqEnqueue(&q->gameplayQueue, e);
        default: return 1; /* invalid queue type */
    }
}

Event dequeueControlEvent(EventQueue *q) {
    Event none;
    memset(&none, 0, sizeof none);
    none.type = EVENT_NONE;
    if (!q) return none;
    return seqDequeue(&q->controlQueue);
}

Event dequeueSystemEvent(EventQueue *q) {
    Event none;
    memset(&none, 0, sizeof none);
    none.type = EVENT_NONE;
    if (!q) return none;
    return seqDequeue(&q->systemQueue);
}

Event dequeueGameplayEvent(EventQueue *q) {
    Event none;
    memset(&none, 0, sizeof none);
    none.type = EVENT_NONE;
    if (!q) return none;
    return seqDequeue(&q->gameplayQueue);
}

int isControlQueueEmpty(const EventQueue *q) {
    if (!q) return 1;
    return seqIsEmpty(&q->controlQueue);
}

int isSystemQueueEmpty(const EventQueue *q) {
    if (!q) return 1;
    return seqIsEmpty(&q->systemQueue);
}

int isGameplayQueueEmpty(const EventQueue *q) {
    if (!q) return 1;
    return seqIsEmpty(&q->gameplayQueue);
}

int isEventQueueEmpty(const EventQueue *q) {
    if (!q) return 1;
    return seqIsEmpty(&q->controlQueue)
        && seqIsEmpty(&q->systemQueue)
        && seqIsEmpty(&q->gameplayQueue);
}