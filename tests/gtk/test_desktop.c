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
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
static LONG WINAPI vectored_handler(EXCEPTION_POINTERS *info) {
    DWORD code = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode : 0;
    void *addr = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionAddress : NULL;
    if (code != 0x406D1388 && code != 0x000006BA && code != 0x40010006) {
        fprintf(stderr, "\n[VEH] Exception 0x%08lX at %p\n", code, addr);
        fflush(stderr);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif
static void on_exit_handler(void) {
    fprintf(stderr, "\n[EXIT] atexit handler called!\n");
    fflush(stderr);
}
static void log_handler(const gchar *log_domain, GLogLevelFlags log_level,
                        const gchar *message, gpointer user_data) {
    (void)user_data;
    fprintf(stderr, "[GLIB %s:0x%x] %s\n", log_domain ? log_domain : "default", (unsigned)log_level, message ? message : "");
    fflush(stderr);
}
#include <signal.h>
static void sig_handler(int sig) {
    fprintf(stderr, "\n[SIGNAL] Received signal %d\n", sig);
    fflush(stderr);
}
static void event_watcher(GdkEvent *event, gpointer data) {
    (void)data;
    printf("[GDK_EVENT] start type=%d\n", event ? event->type : -1); fflush(stdout);
    gtk_main_do_event(event);
    printf("[GDK_EVENT] end type=%d\n", event ? event->type : -1); fflush(stdout);
}
static void pump(void) {
    printf("[pump] start\n"); fflush(stdout);
    int iterations = 0;
    while (1) {
        printf("[pump] before iteration %d\n", iterations + 1); fflush(stdout);
        gboolean more = g_main_context_iteration(NULL, FALSE);
        printf("[pump] iteration %d returned %d\n", iterations + 1, (int)more); fflush(stdout);
        if (!more)
            break;
        ++iterations;
    }
    printf("[pump] end after %d iterations\n", iterations); fflush(stdout);
}
int main(int argc, char **argv) {
#ifdef _WIN32
    AddVectoredExceptionHandler(1, vectored_handler);
#endif
    signal(SIGABRT, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGFPE, sig_handler);
    atexit(on_exit_handler);
    g_log_set_default_handler(log_handler, NULL);
    printf("[test_desktop] starting\n"); fflush(stdout);
    test_platform();
    printf("[test_desktop] platform ok\n"); fflush(stdout);
    printf("[test_desktop] calling gui_create\n"); fflush(stdout);
    Gui *g = gui_create(&argc, &argv);
    printf("[test_desktop] gui_create returned %p\n", (void *)g); fflush(stdout);
    assert(g);
    gdk_event_handler_set(event_watcher, NULL, NULL);
    printf("[test_desktop] calling gui_sync\n"); fflush(stdout);
    gui_sync(g);
    printf("[test_desktop] gui_sync returned\n"); fflush(stdout);
    printf("[test_desktop] calling gtk_widget_show_all\n"); fflush(stdout);
    gtk_widget_show_all(g->window);
    printf("[test_desktop] gtk_widget_show_all returned\n"); fflush(stdout);
    printf("[test_desktop] calling pump\n"); fflush(stdout);
    pump();
    printf("[test_desktop] pump returned\n"); fflush(stdout);
    printf("[test_desktop] window shown ok\n"); fflush(stdout);
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
    printf("[test_desktop] hint ok\n"); fflush(stdout);
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
    printf("[test_desktop] ai modes ok\n"); fflush(stdout);
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
    printf("[test_desktop] rapid starts ok\n"); fflush(stdout);
    ac_init_game_config_for_mode(&c, AC_MODE_COMPUTER_VS_COMPUTER);
    assert(!gui_start_game(g, &c, &error));
    gui_sync(g);
    assert(g->ai_job);
    printf("[test_desktop] destroying window\n"); fflush(stdout);
    gtk_widget_destroy(g->window);
    printf("[test_desktop] window destroyed, calling gui_destroy\n"); fflush(stdout);
    gui_destroy(g);
    printf("[test_desktop] gui_destroy done, returning 0\n"); fflush(stdout);
    return 0;
}
