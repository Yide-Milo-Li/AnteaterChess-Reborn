#include "ai/ai.h"
#include "ui/gui.h"

#include <stddef.h>

static int main_generate_ai_move(const GameState *state, Move *move, void *context) {
    (void)context;
    return generateAIMove(state, move);
}

static int main_generate_hint_move(const GameState *state, Move *move, void *context) {
    (void)context;
    return generateHintMove(state, move);
}

int main(int argc, char **argv) {
    Gui *gui = gui_create(&argc, &argv);

    if (gui == NULL) {
        return 1;
    }

    gui_set_move_provider(gui, main_generate_ai_move, NULL);
    gui_set_hint_provider(gui, main_generate_hint_move, NULL);
    gui_run(gui);
    gui_destroy(gui);
    return 0;
}
