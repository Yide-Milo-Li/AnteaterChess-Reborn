#include "anteater/session.hpp"
#include <assert.h>

using namespace ac;
static_assert(value(Difficulty::Tournament) == 5, "Tournament must retain its value");
static int64_t now(void *unused) {
    (void)unused;
    return 0;
}
int main(void) {
    SessionOptions options = {};
    options.clock = Clock{now, NULL};
    Session *session = session_create(&options);
    assert(session);
    GameConfig config;
    init_game_config_for_mode(&config, GameMode::HumanVsComputer);
    Difficulty levels[] = {Difficulty::Easy, Difficulty::Medium, Difficulty::Hard, Difficulty::Tournament};
    for (unsigned i = 0; i < sizeof(levels) / sizeof(levels[0]); ++i) {
        config.aiDifficultyBlack = levels[i];
        assert(session_start(session, &config) == Status::Ok);
    }
    Snapshot before, after;
    session_snapshot(session, &before);
    int invalid[] = {-1, 4, 6};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        config.aiDifficultyBlack = (Difficulty)invalid[i];
        assert(session_start(session, &config) == Status::InvalidArgument);
        session_snapshot(session, &after);
        assert(after.revision == before.revision && after.gameId == before.gameId);
        assert(after.position.hash == before.position.hash);
    }
    config.aiDifficultyBlack = Difficulty::Easy;
    config.aiDifficultyWhite = (Difficulty)4;
    assert(session_start(session, &config) == Status::InvalidArgument);
    session_destroy(session);
    return 0;
}
