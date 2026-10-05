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
    auto owner = Session::create(options);
    Session *session = std::get_if<Session>(&owner);
    assert(session);
    GameConfig config;
    init_game_config_for_mode(&config, GameMode::HumanVsComputer);
    Difficulty levels[] = {Difficulty::Easy, Difficulty::Medium, Difficulty::Hard, Difficulty::Tournament};
    for (unsigned i = 0; i < sizeof(levels) / sizeof(levels[0]); ++i) {
        config.aiDifficultyBlack = levels[i];
        assert(session->start(config) == Status::Ok);
    }
    SessionState before, after;
    before = session->state();
    int invalid[] = {-1, 4, 6};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        config.aiDifficultyBlack = (Difficulty)invalid[i];
        assert(session->start(config) == Status::InvalidArgument);
        after = session->state();
        assert(after.revision == before.revision && after.gameId == before.gameId);
        assert(after.position.hash == before.position.hash);
    }
    config.aiDifficultyBlack = Difficulty::Easy;
    config.aiDifficultyWhite = (Difficulty)4;
    assert(session->start(config) == Status::InvalidArgument);
    return 0;
}
