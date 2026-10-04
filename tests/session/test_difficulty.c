#include "anteater/session.h"
#include <assert.h>
_Static_assert(AC_DIFFICULTY_TOURNAMENT == 5, "Tournament must retain its value");
static int64_t now(void *unused) { (void)unused; return 0; }
int main(void) {
    AcSessionOptions options = {0}; options.clock = (AcClock){now,NULL};
    AcSession *session = ac_session_create(&options); assert(session);
    AcGameConfig config;
    ac_init_game_config_for_mode(&config,AC_MODE_HUMAN_VS_COMPUTER);
    AcAIDifficulty levels[] = {AC_DIFFICULTY_EASY,AC_DIFFICULTY_MEDIUM,AC_DIFFICULTY_HARD,AC_DIFFICULTY_TOURNAMENT};
    for (unsigned i=0; i<sizeof(levels)/sizeof(levels[0]); ++i) {
        config.aiDifficultyBlack = levels[i];
        assert(ac_session_start(session,&config) == AC_OK);
    }
    AcSnapshot before,after;
    ac_session_snapshot(session,&before);
    int invalid[] = {-1,4,6};
    for (unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i) {
        config.aiDifficultyBlack = (AcAIDifficulty)invalid[i];
        assert(ac_session_start(session,&config) == AC_INVALID_ARGUMENT);
        ac_session_snapshot(session,&after);
        assert(after.revision == before.revision && after.gameId == before.gameId);
        assert(after.position.hash == before.position.hash);
    }
    config.aiDifficultyBlack = AC_DIFFICULTY_EASY;
    config.aiDifficultyWhite = (AcAIDifficulty)4;
    assert(ac_session_start(session,&config) == AC_INVALID_ARGUMENT);
    ac_session_destroy(session);
    return 0;
}
