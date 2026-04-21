#include "ui/gameplay_ui.h"
#include "ui/gui.h"
#include "ui/board_renderer.h"

int displayGameStatus(const GameState *state){
    Gui *gui = get_board_renderer_gui();
    if (!gui) return -1;
    update_board(gui, state);
    update_movelist(gui, state);
    update_clock(gui);
    update_timers(gui, state);
    displayTurn(state->currentTurn);
    return 0;
}
int displayTurn(Color turn){
    Gui *gui = get_board_renderer_gui();
    if (!gui) return -1;
    gui_display_turn(gui, turn);
    return 0;
}