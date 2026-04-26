#include <assert.h>
#include <string.h>

#include "core/move.h"
#include "error/error_code.h"
#include "system/event.h"
#include "system/event_queue.h"

/* Check that the public event constructors fill only the documented payload. */
static void test_event_constructors(void) {
    Move move;
    Event event;

    memset(&move, 0, sizeof(move));
    move.from = createPosition(6, 0);
    move.to = createPosition(4, 0);
    event = createAIMoveEvent(move);
    assert(event.type == EVENT_AI_MOVE);
    assert(positionEqual(event.data.move.from, createPosition(6, 0)) == 1);
    assert(positionEqual(event.data.move.to, createPosition(4, 0)) == 1);

    event = createPlayerMoveEvent(move);
    assert(event.type == EVENT_PLAYER_MOVE);
    assert(positionEqual(event.data.move.from, createPosition(6, 0)) == 1);
    assert(positionEqual(event.data.move.to, createPosition(4, 0)) == 1);

    assert(createSystemEvent(EVENT_BACK).type == EVENT_BACK);
    assert(createSystemEvent(EVENT_EXIT_PROGRAM).type == EVENT_EXIT_PROGRAM);
    assert(createSystemEvent(EVENT_PLAYER_MOVE).type == EVENT_NONE);

    event = createUndoEvent();
    assert(event.type == EVENT_UNDO);

    event = createErrorEvent(ERR_ILLEGAL_MOVE);
    assert(event.type == EVENT_ERROR);
    assert(event.data.errorCode == ERR_ILLEGAL_MOVE);

    event = createFatalErrorEvent(ERR_FATAL);
    assert(event.type == EVENT_FATAL_ERROR);
    assert(event.data.errorCode == ERR_FATAL);
}

/* Check that zero-initialized queues satisfy the public empty-state contract. */
static void test_zero_initialized_queue_is_empty(void) {
    EventQueue queue = {0};

    assert(isControlQueueEmpty(&queue) == 1);
    assert(isSystemQueueEmpty(&queue) == 1);
    assert(isGameplayQueueEmpty(&queue) == 1);
    assert(dequeueControlEvent(&queue).type == EVENT_NONE);
    assert(dequeueSystemEvent(&queue).type == EVENT_NONE);
    assert(dequeueGameplayEvent(&queue).type == EVENT_NONE);
}

/* Check that FIFO order is preserved independently inside each public subqueue. */
static void test_queue_fifo_and_independence(void) {
    EventQueue queue = {0};
    Event firstError = createErrorEvent(ERR_INVALID_INPUT);
    Event secondError = createErrorEvent(ERR_UNDO_UNAVAILABLE);
    Event gameplayEvent = createUndoEvent();
    Event systemEvent = createSystemEvent(EVENT_TIMER_EXPIRED);

    assert(enqueueEvent(&queue, firstError, QUEUE_CONTROL) == 0);
    assert(enqueueEvent(&queue, secondError, QUEUE_CONTROL) == 0);
    assert(enqueueEvent(&queue, gameplayEvent, QUEUE_GAMEPLAY) == 0);
    assert(enqueueEvent(&queue, systemEvent, QUEUE_SYSTEM) == 0);

    assert(dequeueControlEvent(&queue).data.errorCode == ERR_INVALID_INPUT);
    assert(dequeueControlEvent(&queue).data.errorCode == ERR_UNDO_UNAVAILABLE);
    assert(dequeueSystemEvent(&queue).type == EVENT_TIMER_EXPIRED);
    assert(dequeueGameplayEvent(&queue).type == EVENT_UNDO);

    assert(isControlQueueEmpty(&queue) == 1);
    assert(isSystemQueueEmpty(&queue) == 1);
    assert(isGameplayQueueEmpty(&queue) == 1);
}

/* Check that the ring buffer supports MAX_EVENTS items and wraps cleanly. */
static void test_queue_capacity_and_wraparound(void) {
    EventQueue queue = {0};
    int index;
    Event event;

    for (index = 0; index < MAX_EVENTS; ++index) {
        assert(enqueueEvent(&queue, createUndoEvent(), QUEUE_GAMEPLAY) == 0);
    }
    assert(enqueueEvent(&queue, createUndoEvent(), QUEUE_GAMEPLAY) != 0);

    for (index = 0; index < MAX_EVENTS / 2; ++index) {
        assert(dequeueGameplayEvent(&queue).type == EVENT_UNDO);
    }

    assert(enqueueEvent(&queue, createErrorEvent(ERR_NOT_YOUR_TURN), QUEUE_GAMEPLAY) == 0);

    for (index = 0; index < MAX_EVENTS / 2; ++index) {
        assert(dequeueGameplayEvent(&queue).type == EVENT_UNDO);
    }

    event = dequeueGameplayEvent(&queue);
    assert(event.type == EVENT_ERROR);
    assert(event.data.errorCode == ERR_NOT_YOUR_TURN);
    assert(isGameplayQueueEmpty(&queue) == 1);
}

/* Run the Phase E event and queue regression suite. */
int main(void) {
    test_event_constructors();
    test_zero_initialized_queue_is_empty();
    test_queue_fifo_and_independence();
    test_queue_capacity_and_wraparound();
    return 0;
}
