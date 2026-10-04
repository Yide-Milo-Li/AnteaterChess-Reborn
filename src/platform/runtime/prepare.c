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
