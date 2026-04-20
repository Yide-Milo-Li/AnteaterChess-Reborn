#include <string.h>

#include "core/gamestate.h"
#include "input/command.h"
#include "system/controller.h"
#include "system/event.h"
#include "system/event_queue.h"
#include "system/fsm.h"
#include "system/system_state.h"

typedef struct {
    Event script[32];
    int   count;
    int   cursor;
} ScriptedPoller;

static ScriptedPoller g_ui_poller;

static int scripted_ui_poll(GameState *state, Event *out) {
    (void)state;
    if (g_ui_poller.cursor >= g_ui_poller.count) return 0;
    *out = g_ui_poller.script[g_ui_poller.cursor++];
    return 1;
}

static void scriptPush(Event e) {
    if (g_ui_poller.count < 32) {
        g_ui_poller.script[g_ui_poller.count++] = e;
    }
}

static void scriptReset(void) {
    memset(&g_ui_poller, 0, sizeof g_ui_poller);
}

static void test_event_queue_routing(void) {
    BEGIN_CASE("queueForEvent routes control events to QUEUE_CONTROL");
    CHECK_EQ(queueForEvent(EVENT_LEAVE_GAME),   QUEUE_CONTROL);
    CHECK_EQ(queueForEvent(EVENT_BACK),         QUEUE_CONTROL);
    CHECK_EQ(queueForEvent(EVENT_EXIT_PROGRAM), QUEUE_CONTROL);
    CHECK_EQ(queueForEvent(EVENT_NEW_GAME),     QUEUE_CONTROL);
    CHECK_EQ(queueForEvent(EVENT_ERROR),        QUEUE_CONTROL);
    CHECK_EQ(queueForEvent(EVENT_FATAL_ERROR),  QUEUE_CONTROL);

    BEGIN_CASE("queueForEvent routes system events to QUEUE_SYSTEM");
    CHECK_EQ(queueForEvent(EVENT_TIMER_EXPIRED), QUEUE_SYSTEM);

    BEGIN_CASE("queueForEvent routes gameplay events to QUEUE_GAMEPLAY");
    CHECK_EQ(queueForEvent(EVENT_MOVE_INPUT), QUEUE_GAMEPLAY);
    CHECK_EQ(queueForEvent(EVENT_AI_MOVE),    QUEUE_GAMEPLAY);
    CHECK_EQ(queueForEvent(EVENT_UNDO),       QUEUE_GAMEPLAY);
}

static void test_priority_ordering(void) {
    BEGIN_CASE("controller processes ControlQueue before other queues");
    GameState s; EventQueue q;
    initGameState(&s);
    initEventQueue(&q);
    initFSM();

    enqueueEvent(&q, createUndoEvent(),                 QUEUE_GAMEPLAY);
    enqueueEvent(&q, createSystemEvent(EVENT_TIMER_EXPIRED), QUEUE_SYSTEM);
    enqueueEvent(&q, createSystemEvent(EVENT_NONE),     QUEUE_CONTROL);
    processEvent(&s, createSystemEvent(EVENT_NONE), &q); /* INIT -> MAIN */
    enqueueEvent(&q, createSystemEvent(EVENT_EXIT_PROGRAM), QUEUE_CONTROL);

    int finalState = tickGameLoop(&s, &q, NULL);
    CHECK_EQ(finalState, EXIT_STATE);
}

static void test_runGameLoop_terminates(void) {
    BEGIN_CASE("runGameLoop terminates when EXIT_PROGRAM is enqueued");
    GameState s; EventQueue q;
    initGameState(&s);
    initEventQueue(&q);
    initFSM();

    enqueueEvent(&q, createSystemEvent(EVENT_EXIT_PROGRAM), QUEUE_CONTROL);
    int rc = runGameLoop(&s, &q, NULL);
    CHECK_EQ(rc, 0);
    CHECK_EQ(getSystemState(), EXIT_STATE);
}

static void test_move_pipeline_success(void) {
    BEGIN_CASE("valid move flows through pipeline and mutates GameState");
    GameState s; EventQueue q;
    initGameState(&s);
    initEventQueue(&q);
    initFSM();

    Piece whitePawn = { ANT, WHITE };
    setPiece(&s.board, createPosition(1, 0), whitePawn);
    s.currentTurn = WHITE;

    /* Build Command for move (1,0) -> (2,0). */
    Command cmd;
    createMoveCommand(&cmd, createPosition(1, 0), createPosition(2, 0));

    int rc = runInputMovePipeline(&s, cmd, &q);
    CHECK_EQ(rc, MOVE_PIPELINE_OK);
    /* Source cleared, destination populated. */
    CHECK_EQ(getPiece(&s.board, createPosition(1, 0)).type, EMPTY_PIECE);
    CHECK_EQ(getPiece(&s.board, createPosition(2, 0)).type, ANT);
    /* Turn switched to BLACK. */
    CHECK_EQ(s.currentTurn, BLACK);
    /* History has one entry. */
    CHECK_EQ(s.history.count, 1);
    /* No error event was enqueued. */
    CHECK(isControlQueueEmpty(&q));
}

static void test_move_pipeline_invalid_raises_error(void) {
    BEGIN_CASE("invalid move raises EVENT_ERROR and does not mutate state");
    GameState s; EventQueue q;
    initGameState(&s);
    initEventQueue(&q);
    initFSM();
    s.currentTurn = WHITE;
    Command cmd;
    createMoveCommand(&cmd, createPosition(0, 0), createPosition(1, 0));

    int rc = runInputMovePipeline(&s, cmd, &q);

    CHECK_EQ(rc, MOVE_PIPELINE_ERR_ILLEGAL_MOVE);
    /* Turn unchanged, history still empty. */
    CHECK_EQ(s.currentTurn, WHITE);
    CHECK_EQ(s.history.count, 0);
    /* An EVENT_ERROR should be waiting on ControlQueue. */
    CHECK(!isControlQueueEmpty(&q));
    Event err = dequeueControlEvent(&q);
    CHECK_EQ(err.type, EVENT_ERROR);
}

static void test_simulated_session(void) {
    BEGIN_CASE("scripted UI poller drives a full session to EXIT");
    GameState s; EventQueue q;
    initGameState(&s);
    initEventQueue(&q);
    initFSM();
    scriptReset();

    scriptPush(createSystemEvent(EVENT_NEW_GAME));     /* MAIN -> MODE     */
    scriptPush(createSystemEvent(EVENT_NEW_GAME));     /* MODE -> SETUP    */
    scriptPush(createSystemEvent(EVENT_NEW_GAME));     /* SETUP -> GAMEPLAY*/

    Command cmd;
    Piece whitePawn = { ANT, WHITE };
    setPiece(&s.board, createPosition(1, 0), whitePawn);
    createMoveCommand(&cmd, createPosition(1, 0), createPosition(2, 0));
    scriptPush(createMoveInputEvent(cmd));

    scriptPush(createSystemEvent(EVENT_LEAVE_GAME));   /* GAMEPLAY -> TERM */
    scriptPush(createSystemEvent(EVENT_BACK));         /* MENU -> MAIN     */
    scriptPush(createSystemEvent(EVENT_EXIT_PROGRAM)); /* MAIN -> EXIT     */

    EventSources sources = {0};
    sources.pollUI = scripted_ui_poll;

    int rc = runGameLoop(&s, &q, &sources);
    CHECK_EQ(rc, 0);
    CHECK_EQ(getSystemState(), EXIT_STATE);
    CHECK_EQ(getPiece(&s.board, createPosition(2, 0)).type, ANT);
}

int main(void) {
    test_event_queue_routing();
    test_priority_ordering();
    test_runGameLoop_terminates();
    test_move_pipeline_success();
    test_move_pipeline_invalid_raises_error();
    test_simulated_session();
    TEST_SUMMARY("test_control_flow");
}