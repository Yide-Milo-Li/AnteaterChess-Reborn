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
        int ok = 1;
        const char *icons[] = {"icon-alert-dark.svg", "icon-hint-dark.svg",
                               "icon-history-dark.svg", "icon-info-dark.svg"};
        for (int color = AC_WHITE; color <= AC_BLACK; ++color)
            for (int type = AC_ANT; type <= AC_ANTEATER; ++type)
                ok &= gui_get_piece_pixbuf(ac_create_piece((AcPieceType)type, (AcColor)color), 32) != NULL;
        for (size_t i = 0; i < G_N_ELEMENTS(icons); ++i) {
            GdkPixbuf *pixbuf = gui_get_ui_icon_pixbuf(icons[i], 32);
            ok &= pixbuf != NULL;
            if (pixbuf)
                g_object_unref(pixbuf);
        }
        /* Exercise the packaged application's real session/log path as well as assets. */
        AcGameConfig config;
        AcErrorCode error;
        AcSnapshot snapshot;
        ac_init_default_game_config(&config);
        ok &= gui_start_game(g, &config, &error) == 0;
        ok &= ac_session_finish(g->session) == AC_OK;
        ok &= ac_session_snapshot(g->session, &snapshot) == AC_OK;
        ok &= snapshot.diagnostic == AC_OK;
        gtk_widget_destroy(g->window);
        gui_destroy(g);
        return ok ? 0 : 1;
    }
    gui_run(g);
    gui_destroy(g);
    return 0;
}
