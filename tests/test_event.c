#include "core/error_code.h"
#include "core/move.h"
#include "input/command.h"
#include "system/event.h"
#include "system/event_queue.h"

static void test_constructors(void) {
    BEGIN_CASE("createMoveInputEvent populates command");
    Command cmd; createSimpleCommand(&cmd, CMD_UNDO);
    cmd.type = CMD_MOVE;
    cmd.from = createPosition(1, 2);
    cmd.to   = createPosition(3, 4);
    Event e = createMoveInputEvent(cmd);
    CHECK_EQ(e.type, EVENT_MOVE_INPUT);
    CHECK_EQ(e.data.command.type, CMD_MOVE);
    CHECK_EQ(e.data.command.from.row, 1);
    CHECK_EQ(e.data.command.to.col,   4);

    BEGIN_CASE("createAIMoveEvent populates move");
    Move m = {0};
    m.from = createPosition(6, 0);
    m.to   = createPosition(4, 0);
    Event ai = createAIMoveEvent(m);
    CHECK_EQ(ai.type, EVENT_AI_MOVE);
    CHECK_EQ(ai.data.move.from.row, 6);
    CHECK_EQ(ai.data.move.to.row,   4);

    BEGIN_CASE("createSystemEvent accepts payload-free types");
    CHECK_EQ(createSystemEvent(EVENT_BACK).type,         EVENT_BACK);
    CHECK_EQ(createSystemEvent(EVENT_EXIT_PROGRAM).type, EVENT_EXIT_PROGRAM);
    CHECK_EQ(createSystemEvent(EVENT_NEW_GAME).type,     EVENT_NEW_GAME);
    CHECK_EQ(createSystemEvent(EVENT_LEAVE_GAME).type,   EVENT_LEAVE_GAME);
    CHECK_EQ(createSystemEvent(EVENT_HINT).type,         EVENT_HINT);

    BEGIN_CASE("createSystemEvent rejects payload-requiring types");
    CHECK_EQ(createSystemEvent(EVENT_MOVE_INPUT).type, EVENT_NONE);
    CHECK_EQ(createSystemEvent(EVENT_ERROR).type,      EVENT_NONE);

    BEGIN_CASE("createUndoEvent has no payload");
    CHECK_EQ(createUndoEvent().type, EVENT_UNDO);

    BEGIN_CASE("createErrorEvent carries errorCode");
    Event err = createErrorEvent(ERR_INVALID_MOVE);
    CHECK_EQ(err.type, EVENT_ERROR);
    CHECK_EQ(err.data.errorCode, ERR_INVALID_MOVE);

    BEGIN_CASE("createFatalErrorEvent carries errorCode");
    Event f = createFatalErrorEvent(ERR_FATAL_STATE_CORRUPT);
    CHECK_EQ(f.type, EVENT_FATAL_ERROR);
    CHECK_EQ(f.data.errorCode, ERR_FATAL_STATE_CORRUPT);
}

static void test_fifo_behavior(void) {
    BEGIN_CASE("initEventQueue leaves all three queues empty");
    EventQueue q;
    initEventQueue(&q);
    CHECK(isControlQueueEmpty(&q));
    CHECK(isSystemQueueEmpty(&q));
    CHECK(isGameplayQueueEmpty(&q));
    CHECK(isEventQueueEmpty(&q));

    BEGIN_CASE("FIFO order is preserved within a queue");
    Event a = createErrorEvent(ERR_INVALID_MOVE);
    Event b = createErrorEvent(ERR_UNDO_UNAVAILABLE);
    Event c = createErrorEvent(ERR_NOT_PLAYER_TURN);
    CHECK_EQ(enqueueEvent(&q, a, QUEUE_CONTROL), 0);
    CHECK_EQ(enqueueEvent(&q, b, QUEUE_CONTROL), 0);
    CHECK_EQ(enqueueEvent(&q, c, QUEUE_CONTROL), 0);

    Event out1 = dequeueControlEvent(&q);
    Event out2 = dequeueControlEvent(&q);
    Event out3 = dequeueControlEvent(&q);
    CHECK_EQ(out1.data.errorCode, ERR_INVALID_MOVE);
    CHECK_EQ(out2.data.errorCode, ERR_UNDO_UNAVAILABLE);
    CHECK_EQ(out3.data.errorCode, ERR_NOT_PLAYER_TURN);

    BEGIN_CASE("queues are independent of each other");
    Event e1 = createSystemEvent(EVENT_TIMER_EXPIRED);
    Event e2 = createSystemEvent(EVENT_NEW_GAME);
    Event e3 = createUndoEvent();
    enqueueEvent(&q, e1, QUEUE_SYSTEM);
    enqueueEvent(&q, e2, QUEUE_CONTROL);
    enqueueEvent(&q, e3, QUEUE_GAMEPLAY);
    CHECK(!isControlQueueEmpty(&q));
    CHECK(!isSystemQueueEmpty(&q));
    CHECK(!isGameplayQueueEmpty(&q));
    CHECK_EQ(dequeueControlEvent(&q).type,  EVENT_NEW_GAME);
    CHECK_EQ(dequeueSystemEvent(&q).type,   EVENT_TIMER_EXPIRED);
    CHECK_EQ(dequeueGameplayEvent(&q).type, EVENT_UNDO);
    CHECK(isEventQueueEmpty(&q));

    BEGIN_CASE("dequeue on empty returns EVENT_NONE");
    CHECK_EQ(dequeueControlEvent(&q).type,  EVENT_NONE);
    CHECK_EQ(dequeueSystemEvent(&q).type,   EVENT_NONE);
    CHECK_EQ(dequeueGameplayEvent(&q).type, EVENT_NONE);
}

static void test_capacity_and_wrap(void) {
    BEGIN_CASE("queue reports full after MAX_EVENTS inserts");
    EventQueue q;
    initEventQueue(&q);
    int rc = 0;
    for (int i = 0; i < MAX_EVENTS; ++i) {
        rc = enqueueEvent(&q, createUndoEvent(), QUEUE_GAMEPLAY);
        CHECK_EQ(rc, 0);
    }
    /* Next enqueue must fail. */
    rc = enqueueEvent(&q, createUndoEvent(), QUEUE_GAMEPLAY);
    CHECK(rc != 0);

    BEGIN_CASE("circular buffer wraps correctly");
    for (int i = 0; i < MAX_EVENTS / 2; ++i) {
        CHECK_EQ(dequeueGameplayEvent(&q).type, EVENT_UNDO);
    }
    Event marker = createErrorEvent(ERR_INVALID_SELECTION);

    CHECK_EQ(enqueueEvent(&q, marker, QUEUE_GAMEPLAY), 0);

    for (int i = 0; i < MAX_EVENTS / 2; ++i) {
        CHECK_EQ(dequeueGameplayEvent(&q).type, EVENT_UNDO);
    }

    Event tail = dequeueGameplayEvent(&q);
    CHECK_EQ(tail.type, EVENT_ERROR);
    CHECK_EQ(tail.data.errorCode, ERR_INVALID_SELECTION);
    CHECK(isGameplayQueueEmpty(&q));
}

int main(void) {
    test_constructors();
    test_fifo_behavior();
    test_capacity_and_wrap();
    TEST_SUMMARY("test_event");
}