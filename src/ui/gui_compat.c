#include "ui/board_renderer.h"
#include "ui/gameplay_ui.h"

#include "gui_internal.h"

static Gui *snapshot_gui = NULL;

static Gui *active_snapshot_gui(void) {
    if (snapshot_gui != NULL && !gui_window_is_valid(snapshot_gui)) {
        gui_destroy(snapshot_gui);
        snapshot_gui = NULL;
    }

    return snapshot_gui;
}

static Gui *get_snapshot_gui(void) {
    int argc = 0;
    char **argv = NULL;

    (void)active_snapshot_gui();
    if (snapshot_gui == NULL) {
        snapshot_gui = gui_create(&argc, &argv);
    }

    return snapshot_gui;
}

int renderBoard(const GameState *state) {
    Gui *gui;

    if (state == NULL) {
        return -1;
    }

    gui = get_snapshot_gui();
    if (gui == NULL) {
        return -1;
    }

    return gui_render_snapshot(gui, state);
}

int displayGameStatus(const GameState *state) {
    Gui *gui;

    if (state == NULL) {
        return -1;
    }

    gui = get_snapshot_gui();
    if (gui == NULL) {
        return -1;
    }

    if (!gui->has_rendered_state || gui_get_state(gui) == NULL) {
        return gui_render_snapshot(gui, state);
    }

    gui_update_board(gui, state);
    gui_update_movelist(gui, state);
    gui_update_clock(gui);
    gui_update_timers(gui, state);
    gui_update_turn_display(gui, state->currentTurn);
    return 0;
}

int displayTurn(Color turn) {
    Gui *gui = active_snapshot_gui();

    if (gui == NULL) {
        return -1;
    }

    gui_update_turn_display(gui, turn);
    return 0;
}
