#include "platform.h"
#include <glib.h>
#ifdef _WIN32
#include <windows.h>
#endif

char *ac_platform_executable_directory(void) {
    char *executable;
#ifdef _WIN32
    wchar_t path[32768];
    DWORD length = GetModuleFileNameW(NULL, path, G_N_ELEMENTS(path));
    if (!length || length >= G_N_ELEMENTS(path))
        return NULL;
    executable = g_utf16_to_utf8((const gunichar2 *)path, length, NULL, NULL, NULL);
#else
    /* The kernel resolves the executable itself, including symlink launches. */
    executable = g_file_read_link("/proc/self/exe", NULL);
#endif
    if (!executable)
        return NULL;
    char *directory = g_path_is_absolute(executable) ? g_path_get_dirname(executable) : NULL;
    g_free(executable);
    return directory;
}
