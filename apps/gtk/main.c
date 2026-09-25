#include "gui_internal.h"
#include <string.h>
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--version")) {
        g_print("AnteaterChess Reborn 2.0.1\n");
        return 0;
    }
    Gui *g = gui_create(&argc, &argv);
    if (!g)
        return 1;
    if (argc == 2 && !strcmp(argv[1], "--smoke-test")) {
        gui_sync(g);
        gtk_widget_show_all(g->window);
        while (g_main_context_iteration(NULL, FALSE))
            ;
        int ok = gui_get_piece_pixbuf(ac_create_piece(AC_ANTEATER, AC_WHITE), 32) != NULL;
        gtk_widget_destroy(g->window);
        gui_destroy(g);
        return ok ? 0 : 1;
    }
    gui_run(g);
    gui_destroy(g);
    return 0;
}
