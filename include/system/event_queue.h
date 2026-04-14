#ifndef CHESS_SYSTEM_EVENT_QUEUE_H
#define CHESS_SYSTEM_EVENT_QUEUE_H

#include "system/event.h"

#define MAX_EVENTS 128

typedef enum {
    QUEUE_CONTROL,
    QUEUE_SYSTEM,
    QUEUE_GAMEPLAY
} QueueType;

typedef struct {
    Event events[MAX_EVENTS];
    int front;
    int rear;
} SingleEventQueue;

typedef struct {
    SingleEventQueue controlQueue;
    SingleEventQueue systemQueue;
    SingleEventQueue gameplayQueue;
} EventQueue;

int enqueueEvent(EventQueue *q, Event e, QueueType type);
Event dequeueControlEvent(EventQueue *q);
Event dequeueSystemEvent(EventQueue *q);
Event dequeueGameplayEvent(EventQueue *q);
int isControlQueueEmpty(const EventQueue *q);
int isSystemQueueEmpty(const EventQueue *q);
int isGameplayQueueEmpty(const EventQueue *q);

#endif
