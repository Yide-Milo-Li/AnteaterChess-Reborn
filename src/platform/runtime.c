#include "platform.h"
#include <glib.h>
#include <glib/gstdio.h>
#ifdef _WIN32
#include <windows.h>
#endif
void ac_platform_prepare_runtime(void) {
#ifdef _WIN32
    wchar_t executable[32768];
    if (!GetModuleFileNameW(NULL, executable, G_N_ELEMENTS(executable)))
        return;
    char *utf8 = g_utf16_to_utf8((const gunichar2 *)executable, -1, NULL, NULL, NULL);
    if (!utf8)
        return;
    char *base = g_path_get_dirname(utf8);
    g_free(utf8);
    char *query = g_build_filename(base, "gdk-pixbuf-query-loaders.exe", NULL);
    if (g_file_test(query, G_FILE_TEST_EXISTS)) {
        char *loader =
            g_build_filename(base, "lib", "gdk-pixbuf-2.0", "2.10.0", "loaders", "libpixbufloader-svg.dll", NULL);
        char *args[] = {query, loader, NULL}, *output = NULL;
        int status;
        if (g_spawn_sync(NULL, args, NULL, 0, NULL, NULL, &output, NULL, &status, NULL) && status == 0) {
            /* query-loaders emits paths relative to its executable on Windows.
             * The writable cache lives elsewhere, so make its module path absolute. */
            char **lines=g_strsplit(output,"\n",-1);
            char *escaped=g_strescape(loader,NULL);
            for(int i=0;lines[i];++i) {
                if(lines[i][0]=='"' && strstr(lines[i],".dll\"")) {
                    g_free(lines[i]);lines[i]=g_strdup_printf("\"%s\"",escaped);
                }
            }
            g_free(escaped);g_free(output);output=g_strjoinv("\n",lines);g_strfreev(lines);
            char *directory = g_build_filename(g_get_user_cache_dir(), "AnteaterChess-Reborn", NULL);
            g_mkdir_with_parents(directory, 0700);
            char *cache = g_build_filename(directory, "loaders.cache", NULL);
            if (g_file_set_contents(cache, output, -1, NULL))
                g_setenv("GDK_PIXBUF_MODULE_FILE", cache, TRUE);
            g_free(cache);
            g_free(directory);
        }
        g_free(output);
        g_free(loader);
        char *share = g_build_filename(base, "share", NULL);
        g_setenv("XDG_DATA_DIRS", share, TRUE);
        g_free(share);
    }
    g_free(query);
    g_free(base);
#endif
}
struct AcPlatformLog {
    char *directory, *path;
    uint64_t lastRevision, lastGameId;
    int lastCount;
    AcSessionPhase lastPhase;
};
int64_t ac_platform_now(void *unused) {
    (void)unused;
    return g_get_monotonic_time() / 1000;
}
AcPlatformLog *ac_platform_log_create(const char *directory) {
    AcPlatformLog *l = g_try_new0(AcPlatformLog, 1);
    if (!l)
        return NULL;
    if (directory)
        l->directory = g_strdup(directory);
    else {
#ifdef _WIN32
        const char *base = g_get_user_data_dir();
#else
        const char *base = g_get_user_state_dir();
#endif
        l->directory = g_build_filename(base, "AnteaterChess-Reborn", "logs", NULL);
    }
    return l;
}
AcStatus ac_platform_log_write(void *context, const AcSnapshot *s) {
    AcPlatformLog *l = context;
    if (!l || !s)
        return AC_IO_ERROR;
    if (!l->path || l->lastGameId != s->gameId) {
        g_free(l->path);
        char *id = g_uuid_string_random();
        char *name = g_strdup_printf("session-%s.log", id);
        l->path = g_build_filename(l->directory, name, NULL);
        g_free(name);
        g_free(id);
    }
    if (g_mkdir_with_parents(l->directory, 0700) != 0)
        return AC_IO_ERROR;
    GString *text = g_string_new("AnteaterChess Reborn 2.0.0\n");
    g_string_append_printf(text, "Mode: %d\nTurn timer: %d seconds (%s)\n", s->config.mode,
                           s->config.initialTimeSeconds, s->config.timerEnabled ? "enabled" : "disabled");
    for (int i = 0; i < s->historyCount; ++i) {
        AcMove m = s->history[i];
        g_string_append_printf(text, "%d. %s %c%d -> %c%d special=%d captures=%d", i + 1,
                               m.movedPiece.color == AC_WHITE ? "White" : "Black", 'A' + m.from.col, 8 - m.from.row,
                               'A' + m.to.col, 8 - m.to.row, m.specialType, m.captureCount);
        for (int k = 0; k < m.captureCount; ++k)
            g_string_append_printf(text, " %c%d", 'A' + m.captures[k].pos.col, 8 - m.captures[k].pos.row);
        g_string_append_c(text, '\n');
    }
    g_string_append_printf(text, "Elapsed ms: %" G_GINT64_FORMAT "\nResult: %d\n", (gint64)s->elapsedMs, s->result);
    GError *error = NULL;
    gboolean ok = g_file_set_contents(l->path, text->str, text->len, &error);
    g_string_free(text, TRUE);
    if (error)
        g_error_free(error);
    l->lastCount = s->historyCount;
    l->lastPhase = s->phase;
    l->lastRevision = s->revision;
    l->lastGameId = s->gameId;
    return ok ? AC_OK : AC_IO_ERROR;
}
const char *ac_platform_log_path(const AcPlatformLog *l) {
    return l ? l->path : NULL;
}
void ac_platform_log_destroy(AcPlatformLog *l) {
    if (l) {
        g_free(l->directory);
        g_free(l->path);
        g_free(l);
    }
}
