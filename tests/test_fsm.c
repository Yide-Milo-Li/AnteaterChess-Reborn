#include "core/gamestate.h"
#include "system/event.h"
#include "system/event_queue.h"
#include "system/fsm.h"
#include "system/system_state.h"

static void resetFSM(GameState *state, EventQueue *q) {
    initGameState(state);
    initEventQueue(q);
    initFSM();
}

static void test_boot_sequence(void) {
    BEGIN_CASE("FSM starts in INIT_STATE");
    GameState s; EventQueue q;
    resetFSM(&s, &q);
    CHECK_EQ(getSystemState(), INIT_STATE);

    BEGIN_CASE("INIT_STATE auto-advances to MAIN_MENU_STATE on any event");
    processEvent(&s, createSystemEvent(EVENT_NONE), &q);
    CHECK_EQ(getSystemState(), MAIN_MENU_STATE);
}

static void test_forward_path(void) {
    BEGIN_CASE("MAIN_MENU -> GAME_MODE_SELECTION on NEW_GAME");
    GameState s; EventQueue q;
    resetFSM(&s, &q);
    processEvent(&s, createSystemEvent(EVENT_NONE), &q); /* leave INIT */
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q);
    CHECK_EQ(getSystemState(), GAME_MODE_SELECTION_STATE);

    BEGIN_CASE("GAME_MODE_SELECTION -> GAME_SETUP on NEW_GAME");
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q);
    CHECK_EQ(getSystemState(), GAME_SETUP_STATE);

    BEGIN_CASE("GAME_SETUP -> GAMEPLAY on NEW_GAME");
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q);
    CHECK_EQ(getSystemState(), GAMEPLAY_STATE);

    BEGIN_CASE("GAMEPLAY -> GAME_TERMINATION on LEAVE_GAME");
    processEvent(&s, createSystemEvent(EVENT_LEAVE_GAME), &q);
    CHECK_EQ(getSystemState(), GAME_TERMINATION_STATE);

    BEGIN_CASE("GAME_TERMINATION auto-advances to END_GAME_MENU");
    processEvent(&s, createSystemEvent(EVENT_NONE), &q);
    CHECK_EQ(getSystemState(), END_GAME_MENU_STATE);

    BEGIN_CASE("END_GAME_MENU -> MAIN_MENU on BACK");
    processEvent(&s, createSystemEvent(EVENT_BACK), &q);
    CHECK_EQ(getSystemState(), MAIN_MENU_STATE);

    BEGIN_CASE("MAIN_MENU -> EXIT on EXIT_PROGRAM");
    processEvent(&s, createSystemEvent(EVENT_EXIT_PROGRAM), &q);
    CHECK_EQ(getSystemState(), EXIT_STATE);
}

static void test_back_paths(void) {
    BEGIN_CASE("GAME_MODE_SELECTION -> MAIN_MENU on BACK");
    GameState s; EventQueue q;
    resetFSM(&s, &q);
    processEvent(&s, createSystemEvent(EVENT_NONE), &q);     /* -> MAIN */
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q); /* -> MODE */
    processEvent(&s, createSystemEvent(EVENT_BACK), &q);
    CHECK_EQ(getSystemState(), MAIN_MENU_STATE);

    BEGIN_CASE("GAME_SETUP -> GAME_MODE_SELECTION on BACK");
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q); /* -> MODE */
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q); /* -> SETUP */
    processEvent(&s, createSystemEvent(EVENT_BACK), &q);
    CHECK_EQ(getSystemState(), GAME_MODE_SELECTION_STATE);
}

static void test_end_game_menu_restart(void) {
    BEGIN_CASE("END_GAME_MENU -> GAME_MODE_SELECTION on NEW_GAME");
    GameState s; EventQueue q;
    resetFSM(&s, &q);
    /* Fast-forward to END_GAME_MENU. */
    processEvent(&s, createSystemEvent(EVENT_NONE), &q);
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q);
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q);
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q); /* -> GAMEPLAY */
    processEvent(&s, createSystemEvent(EVENT_LEAVE_GAME), &q); /* -> TERM */
    processEvent(&s, createSystemEvent(EVENT_NONE), &q);       /* -> MENU */
    CHECK_EQ(getSystemState(), END_GAME_MENU_STATE);
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q);
    CHECK_EQ(getSystemState(), GAME_MODE_SELECTION_STATE);
}

static void test_invalid_transitions_refused(void) {
    BEGIN_CASE("direct transition MAIN_MENU -> GAMEPLAY is refused");
    GameState s; EventQueue q;
    resetFSM(&s, &q);
    processEvent(&s, createSystemEvent(EVENT_NONE), &q);
    CHECK_EQ(getSystemState(), MAIN_MENU_STATE);
    /* Explicitly attempt an illegal transition. */
    int rc = transitionState(&s, GAMEPLAY_STATE);
    CHECK(rc != 0);
    CHECK_EQ(getSystemState(), MAIN_MENU_STATE);

    BEGIN_CASE("direct transition GAMEPLAY -> MAIN_MENU is refused");
    /* Navigate legitimately into GAMEPLAY first. */
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q);
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q);
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q);
    CHECK_EQ(getSystemState(), GAMEPLAY_STATE);
    rc = transitionState(&s, MAIN_MENU_STATE);
    CHECK(rc != 0);
    CHECK_EQ(getSystemState(), GAMEPLAY_STATE);
}

static void test_fatal_error_short_circuit(void) {
    BEGIN_CASE("EVENT_FATAL_ERROR jumps to EXIT from MAIN_MENU");
    GameState s; EventQueue q;
    resetFSM(&s, &q);
    processEvent(&s, createSystemEvent(EVENT_NONE), &q);
    CHECK_EQ(getSystemState(), MAIN_MENU_STATE);
    processEvent(&s, createFatalErrorEvent(ERR_FATAL_INTERNAL), &q);
    CHECK_EQ(getSystemState(), EXIT_STATE);

    BEGIN_CASE("EVENT_FATAL_ERROR jumps to EXIT from GAMEPLAY");
    resetFSM(&s, &q);
    processEvent(&s, createSystemEvent(EVENT_NONE), &q);
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q);
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q);
    processEvent(&s, createSystemEvent(EVENT_NEW_GAME), &q);
    CHECK_EQ(getSystemState(), GAMEPLAY_STATE);
    processEvent(&s, createFatalErrorEvent(ERR_FATAL_IO), &q);
    CHECK_EQ(getSystemState(), EXIT_STATE);
}

int main(void) {
    test_boot_sequence();
    test_forward_path();
    test_back_paths();
    test_end_game_menu_restart();
    test_invalid_transitions_refused();
    test_fatal_error_short_circuit();
    TEST_SUMMARY("test_fsm");
}