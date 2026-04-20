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

static ScriptedPoller g_ui;
static ScriptedPoller g_ai;
static ScriptedPoller g_timer;

static int ui_poll(GameState *state, Event *out) {
    (void)state;
    if (g_ui.cursor >= g_ui.count) return 0;
    *out = g_ui.script[g_ui.cursor++];
    return 1;
}

static int ai_poll(GameState *state, Event *out) {
    (void)state;
    if (g_ai.cursor >= g_ai.count) return 0;
    *out = g_ai.script[g_ai.cursor++];
    return 1;
}

static int timer_poll(GameState *state, Event *out) {
    (void)state;
    if (g_timer.cursor >= g_timer.count) return 0;
    *out = g_timer.script[g_timer.cursor++];
    return 1;
}

static void resetScripts(void) {
    memset(&g_ui,    0, sizeof g_ui);
    memset(&g_ai,    0, sizeof g_ai);
    memset(&g_timer, 0, sizeof g_timer);
}

static void pushUI   (Event e) { if (g_ui.count    < 32) g_ui.script[g_ui.count++]       = e; }
static void pushAI   (Event e) { if (g_ai.count    < 32) g_ai.script[g_ai.count++]       = e; }
static void pushTimer(Event e) { if (g_timer.count < 32) g_timer.script[g_timer.count++] = e; }

static void test_queue_routing_control_events(void) {
    BEGIN_CASE("queueForEvent routes control events to QUEUE_CONTROL");
    CHECK_EQ(queueForEvent(EVENT_LEAVE_GAME),   QUEUE_CONTROL);
    CHECK_EQ(queueForEvent(EVENT_BACK),         QUEUE_CONTROL);
    CHECK_EQ(queueForEvent(EVENT_EXIT_PROGRAM), QUEUE_CONTROL);
    CHECK_EQ(queueForEvent(EVENT_NEW_GAME),     QUEUE_CONTROL);
    CHECK_EQ(queueForEvent(EVENT_ERROR),        QUEUE_CONTROL);
    CHECK_EQ(queueForEvent(EVENT_FATAL_ERROR),  QUEUE_CONTROL);
}

static void test_queue_routing_system_events(void) {
    BEGIN_CASE("queueForEvent routes system events to QUEUE_SYSTEM");
    CHECK_EQ(queueForEvent(EVENT_TIMER_EXPIRED), QUEUE_SYSTEM);
}

static void test_queue_routing_gameplay_events(void) {
    BEGIN_CASE("queueForEvent routes gameplay events to QUEUE_GAMEPLAY");
    CHECK_EQ(queueForEvent(EVENT_MOVE_INPUT), QUEUE_GAMEPLAY);
    CHECK_EQ(queueForEvent(EVENT_AI_MOVE),    QUEUE_GAMEPLAY);
    CHECK_EQ(queueForEvent(EVENT_UNDO),       QUEUE_GAMEPLAY);
    CHECK_EQ(queueForEvent(EVENT_HINT),       QUEUE_GAMEPLAY);
}

static void test_init_auto_advances_on_first_tick(void) {
    BEGIN_CASE("tickGameLoop advances INIT_STATE -> MAIN_MENU_STATE");
    GameState s; EventQueue q;
    initGameState(&s);
    initEventQueue(&q);
    initFSM();
    CHECK_EQ(getSystemState(), INIT_STATE);

    tickGameLoop(&s, &q, NULL);
    CHECK_EQ(getSystemState(), MAIN_MENU_STATE);
}

static void test_priority_ordering(void) {
    BEGIN_CASE("controller drains ControlQueue before System and Gameplay queues");
    GameState s; EventQueue q;
    initGameState(&s);
    initEventQueue(&q);
    initFSM();
    tickGameLoop(&s, &q, NULL);
    CHECK_EQ(getSystemState(), MAIN_MENU_STATE);

    enqueueEvent(&q, createUndoEvent(),                      QUEUE_GAMEPLAY);
    enqueueEvent(&q, createSystemEvent(EVENT_TIMER_EXPIRED), QUEUE_SYSTEM);
    enqueueEvent(&q, createSystemEvent(EVENT_EXIT_PROGRAM),  QUEUE_CONTROL);
    tickGameLoop(&s, &q, NULL);
    CHECK_EQ(getSystemState(), EXIT_STATE);
}

static void test_runGameLoop_terminates_on_exit(void) {
    BEGIN_CASE("runGameLoop terminates when EXIT_PROGRAM reaches the FSM");
    GameState s; EventQueue q;
    initGameState(&s);
    initEventQueue(&q);
    initFSM();

    enqueueEvent(&q, createSystemEvent(EVENT_EXIT_PROGRAM), QUEUE_CONTROL);
    int rc = runGameLoop(&s, &q, NULL);
    CHECK_EQ(rc, 0);
    CHECK_EQ(getSystemState(), EXIT_STATE);
}

static void test_runGameLoop_rejects_null_args(void) {
    BEGIN_CASE("runGameLoop returns non-zero on NULL state");
    EventQueue q;
    initEventQueue(&q);
    CHECK(runGameLoop(NULL, &q, NULL) != 0);

    BEGIN_CASE("runGameLoop returns non-zero on NULL queue");
    GameState s;
    initGameState(&s);
    CHECK(runGameLoop(&s, NULL, NULL) != 0);
}

static void test_ui_poller_drives_menu_navigation(void) {
    BEGIN_CASE("UI poller can drive MAIN_MENU -> MODE -> SETUP -> GAMEPLAY");
    GameState s; EventQueue q;
    initGameState(&s);
    initEventQueue(&q);
    initFSM();
    resetScripts();

    pushUI(createSystemEvent(EVENT_NEW_GAME));     /* MAIN -> MODE     */
    pushUI(createSystemEvent(EVENT_NEW_GAME));     /* MODE -> SETUP    */
    pushUI(createSystemEvent(EVENT_NEW_GAME));     /* SETUP -> GAMEPLAY*/

    EventSources sources = {0};
    sources.pollUI = ui_poll;

    tickGameLoop(&s, &q, &sources);  /* INIT -> MAIN */
    tickGameLoop(&s, &q, &sources);  /* MAIN -> MODE */
    tickGameLoop(&s, &q, &sources);  /* MODE -> SETUP */
    tickGameLoop(&s, &q, &sources);  /* SETUP -> GAMEPLAY */
    CHECK_EQ(getSystemState(), GAMEPLAY_STATE);
}

static void test_timer_poller_ends_game(void) {
    BEGIN_CASE("Timer-expired event from poller terminates gameplay");
    GameState s; EventQueue q;
    initGameState(&s);
    initEventQueue(&q);
    initFSM();
    resetScripts();

    pushUI(createSystemEvent(EVENT_NEW_GAME));
    pushUI(createSystemEvent(EVENT_NEW_GAME));
    pushUI(createSystemEvent(EVENT_NEW_GAME));
    pushTimer(createSystemEvent(EVENT_TIMER_EXPIRED));

    EventSources sources = {0};
    sources.pollUI    = ui_poll;
    sources.pollTimer = timer_poll;

    for (int i = 0; i < 5; ++i) {
        tickGameLoop(&s, &q, &sources);
    }

    /* Timer expiry -> GAME_TERMINATION_STATE -> END_GAME_MENU_STATE. */
    CHECK_EQ(getSystemState(), END_GAME_MENU_STATE);
}

static void test_no_pollers_no_events(void) {
    BEGIN_CASE("loop with NULL EventSources performs clean init handshake");
    GameState s; EventQueue q;
    initGameState(&s);
    initEventQueue(&q);
    initFSM();

    for (int i = 0; i < 10; ++i) {
        tickGameLoop(&s, &q, NULL);
    }
    CHECK_EQ(getSystemState(), MAIN_MENU_STATE);
    CHECK(isEventQueueEmpty(&q));
}

static void test_ai_poller_event_routing(void) {
    BEGIN_CASE("AI poller events land on the gameplay queue");
    GameState s; EventQueue q;
    initGameState(&s);
    initEventQueue(&q);
    initFSM();
    resetScripts();

    Move m; memset(&m, 0, sizeof m);
    m.from = createPosition(1, 0);
    m.to   = createPosition(2, 0);
    pushAI(createAIMoveEvent(m));

    EventSources sources = {0};
    sources.pollAI = ai_poll;

    tickGameLoop(&s, &q, NULL); /* leave INIT */
    CHECK_EQ(getSystemState(), MAIN_MENU_STATE);

    tickGameLoop(&s, &q, &sources);
    CHECK(isGameplayQueueEmpty(&q));  /* drained, not left stale */
    CHECK_EQ(getSystemState(), MAIN_MENU_STATE);
}

int main(void) {
    test_queue_routing_control_events();
    test_queue_routing_system_events();
    test_queue_routing_gameplay_events();
    test_init_auto_advances_on_first_tick();
    test_priority_ordering();
    test_runGameLoop_terminates_on_exit();
    test_runGameLoop_rejects_null_args();
    test_ui_poller_drives_menu_navigation();
    test_timer_poller_ends_game();
    test_no_pollers_no_events();
    test_ai_poller_event_routing();
    TEST_SUMMARY("test_controller");
}