#include "platform.h"
#include <glib.h>
#include <glib/gstdio.h>
#ifdef _WIN32
#include <windows.h>
#include <stdint.h>

static void ac_platform_fix_cairo_thread_data(void) {
    HMODULE cairo = GetModuleHandleW(L"libcairo-2.dll");
    if (!cairo)
        cairo = LoadLibraryW(L"libcairo-2.dll");
    if (!cairo)
        return;

    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)cairo;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE *)cairo + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return;
    PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(nt);
    const BYTE *text = NULL;
    DWORD text_size = 0;
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (memcmp(sec[i].Name, ".text", 5) == 0) {
            text = (const BYTE *)cairo + sec[i].VirtualAddress;
            text_size = sec[i].Misc.VirtualSize;
            break;
        }
    }
    if (!text || text_size < 64)
        return;

    /* Cairo 1.18.6 on MinGW has an upstream issue where DllMain in
     * cairo-win32-system.c is not invoked by the MinGW CRT startup stub,
     * leaving _cairo_win32_thread_data_init uncalled.
     * We scan .text for the signature of _cairo_win32_thread_data_init
     * and invoke it if tls_index is uninitialized (-1). */
    for (DWORD i = 0; i + 64 < text_size; ++i) {
        if (text[i] == 0x41 && text[i + 1] == 0xb8 && text[i + 2] == 0x91 && text[i + 3] == 0x00 &&
            text[i + 4] == 0x00 && text[i + 5] == 0x00) {
            const BYTE *fn = text + i - 13;
            if (fn[0] == 0x48 && fn[1] == 0x83 && fn[2] == 0xec && fn[3] == 0x28 &&
                fn[4] == 0x83 && fn[5] == 0x3d) {
                int32_t disp = *(const int32_t *)(fn + 6);
                const int *tls_index_ptr = (const int *)(fn + 11 + disp);
                if (*tls_index_ptr == -1) {
                    typedef void (*InitFn)(void);
                    InitFn init = (InitFn)(uintptr_t)fn;
                    init();
                }
                return;
            }
        }
    }
}
#endif
void ac_platform_prepare_runtime(void) {
#ifdef _WIN32
    ac_platform_fix_cairo_thread_data();
    wchar_t executable[32768];
    if (!GetModuleFileNameW(NULL, executable, G_N_ELEMENTS(executable)))
        return;
    char *utf8 = g_utf16_to_utf8((const gunichar2 *)executable, -1, NULL, NULL, NULL);
    if (!utf8)
        return;
    char *base = g_path_get_dirname(utf8);
    g_free(utf8);
    char *cache = g_build_filename(base, "lib", "gdk-pixbuf-2.0", "2.10.0", "loaders.cache", NULL);
    if (g_file_test(cache, G_FILE_TEST_EXISTS))
        g_setenv("GDK_PIXBUF_MODULE_FILE", cache, TRUE);
    g_free(cache);
    char *share = g_build_filename(base, "share", NULL);
    if (g_file_test(share, G_FILE_TEST_IS_DIR))
        g_setenv("XDG_DATA_DIRS", share, TRUE);
    g_free(share);
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
