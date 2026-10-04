#include "gui_internal.h"
#include <assert.h>
#include <glib/gstdio.h>
static void test_platform(void) {
    char *directory = g_dir_make_tmp("anteater-log-XXXXXX", NULL);
    assert(directory);
    AcPlatformLog *a = ac_platform_log_create(directory), *b = ac_platform_log_create(directory);
    AcSnapshot snapshot = {0};
    snapshot.gameId = 1;
    assert(ac_platform_log_write(a, &snapshot) == AC_OK);
    assert(ac_platform_log_write(b, &snapshot) == AC_OK);
    assert(strcmp(ac_platform_log_path(a), ac_platform_log_path(b)));
    char *first = g_strdup(ac_platform_log_path(a));
    ++snapshot.gameId;
    assert(ac_platform_log_write(a, &snapshot) == AC_OK);
    assert(strcmp(first, ac_platform_log_path(a)));
    AcPlatformLog *invalid = ac_platform_log_create(first);
    assert(ac_platform_log_write(invalid, &snapshot) == AC_IO_ERROR);
    ac_platform_log_destroy(invalid);
    assert(ac_platform_now(NULL) > 0);
    g_remove(first);
    g_free(first);
    g_remove(ac_platform_log_path(a));
    g_remove(ac_platform_log_path(b));
    ac_platform_log_destroy(a);
    ac_platform_log_destroy(b);
    g_rmdir(directory);
    g_free(directory);
}
static void pump(void) {
    while (g_main_context_iteration(NULL, FALSE)) {
    }
}

static void test_resources(void) {
    const int sizes[] = {32, 96, 160};
    const char *icons[] = {"icon-alert-dark.svg", "icon-hint-dark.svg",
                           "icon-history-dark.svg", "icon-info-dark.svg"};
    /* Physical asset moves must preserve every existing GResource alias. */
    for (size_t i = 0; i < G_N_ELEMENTS(sizes); ++i) {
        for (int color = AC_WHITE; color <= AC_BLACK; ++color)
            for (int type = AC_ANT; type <= AC_ANTEATER; ++type)
                assert(gui_get_piece_pixbuf(ac_create_piece((AcPieceType)type, (AcColor)color), sizes[i]));
        for (size_t k = 0; k < G_N_ELEMENTS(icons); ++k) {
            GdkPixbuf *pixbuf = gui_get_ui_icon_pixbuf(icons[k], sizes[i]);
            assert(pixbuf);
            g_object_unref(pixbuf);
        }
    }
}

int main(int argc, char **argv) {
    test_platform();
    Gui *g = gui_create(&argc, &argv);
    assert(g);
    test_resources();
    gui_sync(g);
    gtk_widget_show_all(g->window);
    pump();
    assert(g->page == AC_MAIN_MENU_STATE);
    gui_on_new_game_clicked(NULL, g);
    assert(g->page == AC_GAME_MODE_SELECTION_STATE);
    assert(gui_get_piece_pixbuf(ac_create_piece(AC_ANT, AC_WHITE), 32));
    AcGameConfig c;
    AcErrorCode error;
    ac_init_default_game_config(&c);
    assert(!gui_start_game(g, &c, &error));
    gui_sync(g);
    pump();
    gtk_entry_set_text(GTK_ENTRY(g->from_entry), "E2");
    gtk_entry_set_text(GTK_ENTRY(g->to_entry), "E4");
    gui_on_submit_move_clicked(NULL, g);
    assert(gui_get_state(g)->moveHistory.count == 1);
    gui_on_undo_clicked(NULL, g);
    assert(gui_get_state(g)->moveHistory.count == 0);
    assert(!gui_start_hint_job(g));
    gint64 hint_deadline = g_get_monotonic_time() + 10000000;
    while (g->hint_job && g_get_monotonic_time() < hint_deadline) {
        pump();
        g_usleep(1000);
    }
    assert(!g->hint_job && g->has_hint_highlight);
    assert(!gui_start_hint_job(g));
    gui_invalidate_async_results(g);
    gui_cancel_async_jobs(g);
    assert(!g->hint_job && !g->has_hint_highlight);
    for (int mode = AC_MODE_HUMAN_VS_COMPUTER; mode <= AC_MODE_COMPUTER_VS_COMPUTER; ++mode) {
        ac_init_game_config_for_mode(&c, (AcGameMode)mode);
        c.playerColor = AC_BLACK;
        assert(!gui_start_game(g, &c, &error));
        gui_sync(g);
        assert(g->ai_job);
        gint64 deadline = g_get_monotonic_time() + 10000000;
        while (gui_get_state(g)->moveHistory.count == 0 && g_get_monotonic_time() < deadline) {
            pump();
            g_usleep(1000);
        }
        assert(gui_get_state(g)->moveHistory.count > 0);
        gui_invalidate_async_results(g);
        ac_session_finish(g->session);
        gui_cancel_async_jobs(g);
    }
    ac_init_game_config_for_mode(&c, AC_MODE_COMPUTER_VS_COMPUTER);
    assert(!gui_start_game(g, &c, &error));
    gui_sync(g);
    assert(g->ai_job);
    ac_init_default_game_config(&c);
    for (int i = 0; i < 5; ++i)
        assert(!gui_start_game(g, &c, &error));
    gui_cancel_async_jobs(g);
    pump();
    assert(gui_get_state(g)->moveHistory.count == 0);
    ac_init_game_config_for_mode(&c, AC_MODE_COMPUTER_VS_COMPUTER);
    assert(!gui_start_game(g, &c, &error));
    gui_sync(g);
    assert(g->ai_job);
    gtk_widget_destroy(g->window);
    gui_destroy(g);
    return 0;
}
