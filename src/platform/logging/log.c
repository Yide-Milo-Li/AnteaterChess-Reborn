#include "platform.h"
#include <glib.h>
#include <glib/gstdio.h>

struct AcPlatformLog {
    char *directory, *path;
    uint64_t lastRevision, lastGameId;
    int lastCount;
    AcSessionPhase lastPhase;
};
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
    GString *text = g_string_new("AnteaterChess Reborn 2.0.1\n");
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
