#include "ui/board_renderer.h"

#include "ui/gui.h"

static Gui *gui = NULL;

Gui *get_board_renderer_gui(void) {
    return gui;
}

int br_process_events(void) {
    return gui_process_events();
}

int br_window_is_valid(Gui *target) {
    return gui_window_is_valid(target);
}

int renderBoard(const GameState *state) {
    int argc = 0;
    char **argv = NULL;

    if (state == NULL) {
        return -1;
    }

    if (gui != NULL && !gui_window_is_valid(gui)) {
        gui_destroy(gui);
        gui = NULL;
    }

    if (gui == NULL) {
        gui = gui_create(&argc, &argv);
        if (gui == NULL) {
            return -1;
        }
    }

    return gui_render_snapshot(gui, state);
}
