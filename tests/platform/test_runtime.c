#include "platform.h"
#include <assert.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

static void test_logs(const char *directory) {
    AcPlatformLog *a = ac_platform_log_create(directory);
    AcPlatformLog *b = ac_platform_log_create(directory);
    assert(a && b);
    AcMove move = ac_create_move(ac_parse_position("E2"), ac_parse_position("E4"),
                                ac_create_piece(AC_ANT, AC_WHITE));
    AcSnapshot snapshot = {0};
    ac_init_default_game_config(&snapshot.config);
    snapshot.gameId = 1;
    snapshot.history = &move;
    snapshot.historyCount = 1;
    assert(ac_platform_log_write(a, &snapshot) == AC_OK);
    assert(ac_platform_log_write(b, &snapshot) == AC_OK);
    assert(strcmp(ac_platform_log_path(a), ac_platform_log_path(b)));
    char *first = g_strdup(ac_platform_log_path(a));
    char *text = NULL;
    assert(g_file_get_contents(first, &text, NULL, NULL));
    assert(strstr(text, "1. White E2 -> E4"));
    g_free(text);

    /* Same-game snapshots replace one file; a new game preserves the old log. */
    snapshot.historyCount = 0;
    assert(ac_platform_log_write(a, &snapshot) == AC_OK);
    assert(!strcmp(first, ac_platform_log_path(a)));
    assert(g_file_get_contents(first, &text, NULL, NULL));
    assert(!strstr(text, "1. White"));
    g_free(text);
    ++snapshot.gameId;
    assert(ac_platform_log_write(a, &snapshot) == AC_OK);
    assert(strcmp(first, ac_platform_log_path(a)));
    assert(g_file_test(first, G_FILE_TEST_IS_REGULAR));

    AcPlatformLog *blocked = ac_platform_log_create(first);
    assert(blocked && ac_platform_log_write(blocked, &snapshot) == AC_IO_ERROR);
    ac_platform_log_destroy(blocked);
    assert(g_remove(first) == 0);
    assert(g_remove(ac_platform_log_path(a)) == 0);
    assert(g_remove(ac_platform_log_path(b)) == 0);
    g_free(first);
    ac_platform_log_destroy(a);
    ac_platform_log_destroy(b);
}

int main(int argc, char **argv) {
    char *directory = ac_platform_executable_directory();
    assert(directory && g_path_is_absolute(directory));
    if (argc == 2) {
        char *expected = g_canonicalize_filename(argv[1], NULL);
        assert(!strcmp(directory, expected));
        g_free(expected);
    }
    char *original = g_get_current_dir();
    char *temporary = g_dir_make_tmp("anteater path 棋-XXXXXX", NULL);
    assert(temporary && g_chdir(temporary) == 0);
    test_logs(temporary);
#ifndef _WIN32
    /* Use the native temporary filesystem: mount permissions can mask chmod on NTFS. */
    if (geteuid() != 0) {
        assert(g_chmod(temporary, 0500) == 0);
        AcPlatformLog *denied = ac_platform_log_create(temporary);
        AcSnapshot unwritable = {0};
        assert(denied && ac_platform_log_write(denied, &unwritable) == AC_IO_ERROR);
        assert(!g_file_test(ac_platform_log_path(denied), G_FILE_TEST_EXISTS));
        ac_platform_log_destroy(denied);
        assert(g_chmod(temporary, 0700) == 0);
    }
#endif

    AcPlatformLog *log = ac_platform_log_create(NULL);
    assert(log && !ac_platform_log_path(log));
    AcSnapshot snapshot = {0};
    assert(ac_platform_log_write(log, &snapshot) == AC_OK);
    char *expected = g_build_filename(directory, "logs", NULL);
    char *actual = g_path_get_dirname(ac_platform_log_path(log));
    assert(!strcmp(expected, actual));
    assert(g_remove(ac_platform_log_path(log)) == 0);
    ac_platform_log_destroy(log);
    g_rmdir(expected);
    assert(ac_platform_now(NULL) > 0);
    assert(g_chdir(original) == 0 && g_rmdir(temporary) == 0);
    g_free(actual);
    g_free(expected);
    g_free(temporary);
    g_free(original);
    g_free(directory);
    return 0;
}
