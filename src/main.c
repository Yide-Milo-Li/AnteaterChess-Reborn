#include "ui/gui.h"

#include <stddef.h>

int main(int argc, char **argv) {
    Gui *gui = gui_create(&argc, &argv);

    if (gui == NULL) {
        return 1;
    }

    gui_run(gui);
    gui_destroy(gui);
    return 0;
}
