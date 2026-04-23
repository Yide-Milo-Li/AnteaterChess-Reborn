#include "ui/gameplay_ui.h"

#include "ui/board_renderer.h"
#include "ui/gui.h"

int displayGameStatus(const GameState *state) {
    Gui *gui = get_board_renderer_gui();

    if (state == NULL) {
        return -1;
    }

    if (gui == NULL) {
        return renderBoard(state);
    }

    update_board(gui, state);
    update_movelist(gui, state);
    update_clock(gui);
    update_timers(gui, state);
    gui_display_turn(gui, state->currentTurn);
    return 0;
}

int displayTurn(Color turn) {
    Gui *gui = get_board_renderer_gui();

    if (gui == NULL) {
        return -1;
    }

    gui_display_turn(gui, turn);
    return 0;
}
