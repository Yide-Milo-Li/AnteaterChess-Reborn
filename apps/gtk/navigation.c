#include "gui_internal.h"
int gui_request_new_game(Gui *g) {
    ac_session_finish(g->session);
    g->page = g->page == AC_GAME_MODE_SELECTION_STATE ? AC_GAME_SETUP_STATE : AC_GAME_MODE_SELECTION_STATE;
    return 0;
}
int gui_request_back(Gui *g) {
    ac_session_finish(g->session);
    g->page = g->page == AC_GAME_SETUP_STATE ? AC_GAME_MODE_SELECTION_STATE : AC_MAIN_MENU_STATE;
    return 0;
}
int gui_request_exit(Gui *g) {
    ac_session_finish(g->session);
    g->page = AC_EXIT_STATE;
    return 0;
}
int gui_start_game(Gui *g, const AcGameConfig *config, AcErrorCode *error) {
    AcStatus s = ac_session_start(g->session, config);
    if (s) {
        *error = AC_ERR_INVALID_AI_TIMER_SETTING;
        return s;
    }
    g->page = AC_GAMEPLAY_STATE;
    g->last_move_count = -1;
    return 0;
}
int gui_submit_request(Gui *g, AcMoveRequest request, AcErrorCode *error) {
    AcStatus s = ac_session_submit(g->session, request);
    *error = s == AC_STALE_RESULT ? AC_ERR_TIME_UP : s == AC_UNAVAILABLE ? AC_ERR_NOT_YOUR_TURN : AC_ERR_ILLEGAL_MOVE;
    return s;
}
