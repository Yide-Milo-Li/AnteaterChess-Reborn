#include "platform.h"
#include <assert.h>

/* Link-time replacement makes unavailable executable paths deterministic. */
char *ac_platform_executable_directory(void) {
    return NULL;
}

int main(void) {
    AcPlatformLog *log = ac_platform_log_create(NULL);
    assert(log);
    AcSessionOptions options = {0};
    options.clock = (AcClock){ac_platform_now, NULL};
    options.log = ac_platform_log_write;
    options.logContext = log;
    AcSession *session = ac_session_create(&options);
    assert(session);
    AcGameConfig config;
    ac_init_default_game_config(&config);
    assert(ac_session_start(session, &config) == AC_OK);
    AcSnapshot snapshot;
    assert(ac_session_snapshot(session, &snapshot) == AC_OK);
    assert(snapshot.diagnostic == AC_IO_ERROR);
    AcMoveRequest request;
    assert(ac_parse_move_request_fields("E2", "E4", AC_PROMOTION_CHOICE_NONE, &request) == 0);
    assert(ac_session_submit(session, request) == AC_OK);
    assert(ac_session_snapshot(session, &snapshot) == AC_OK);
    assert(snapshot.historyCount == 1 && snapshot.diagnostic == AC_IO_ERROR);
    assert(!ac_platform_log_path(log));
    ac_session_destroy(session);
    ac_platform_log_destroy(log);
    return 0;
}
