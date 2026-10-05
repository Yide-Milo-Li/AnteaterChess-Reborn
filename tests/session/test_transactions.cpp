#include "anteater/session.hpp"
#include "../core/failing_resource.hpp"
#include <algorithm>
#include <cassert>
#include <climits>

using namespace ac;
static int64_t now(void *context) {
    return *static_cast<int64_t *>(context);
}
static MoveRequest request(const char *from, const char *to) {
    MoveRequest result{};
    assert(parse_move_request_fields(from, to, PromotionChoice::None, &result) == Status::Ok);
    return result;
}
int main() {
    TournamentBudget extremes{};
    initialize_tournament_budget(&extremes);
    charge_tournament_budget(&extremes, Color::White, 7000, 0);
    charge_tournament_budget(&extremes, Color::White, INT_MAX, 0);
    assert(extremes.poolMs[0] == 180000 && extremes.poolMs[1] == 0);
    charge_tournament_budget(&extremes, Color::White, 1, INT_MAX);
    assert(extremes.poolMs[0] == 0 && tournament_expired(&extremes, Color::White));
    FailingResource resource;
    int64_t clock = 0;
    auto owner = Session::create(SessionOptions{
        {now, &clock},
        &resource
    });
    auto &session = std::get<Session>(owner);
    GameConfig config{};
    init_game_config_for_mode(&config, GameMode::ComputerVsComputer);
    config.aiDifficultyWhite = config.aiDifficultyBlack = Difficulty::Tournament;
    assert(session.start(config) == Status::Ok);
    const auto before = session.state();
    const int budget = session.ai_budget();
    const auto liveBefore = resource.live;
    Move move{};
    assert(resolve_move_request(&before.position, request("E2", "E4"), &move) == Status::Ok);
    bool completed = false;
    for (std::size_t failure = 1; failure < 64 && !completed; ++failure) {
        resource.failAt = resource.attempts + failure;
        const auto status = session.submit_ai(move, before.revision, budget, 42);
        if (status == Status::Ok)
            completed = true;
        else {
            assert(status == Status::OutOfMemory && session.state() == before);
            assert(session.ai_budget() == budget && resource.live == liveBefore);
            auto owned = session.snapshot(std::pmr::new_delete_resource());
            const auto &unchanged = std::get<SessionSnapshot>(owned);
            assert(unchanged.history.empty() && unchanged.hashes.size() == 1);
            assert(unchanged.hashes.back() == before.position.hash);
        }
    }
    assert(completed);
    resource.failAt = 0;
    const auto charged = session.state().tournamentRemainingMs;
    assert(charged[0] == before.tournamentRemainingMs[0] - 42);
    assert(session.undo() == Status::Ok);
    assert(session.state().tournamentRemainingMs == charged); // No refund on undo.

    init_default_game_config(&config);
    config.timerEnabled = true;
    config.initialTimeSeconds = 1;
    assert(session.start(config) == Status::Ok);
    const auto initial = session.state();
    resource.failNext();
    auto promotion = session.promotion(request("E2", "E4"));
    assert(std::get<Error>(promotion).status == Status::OutOfMemory);
    assert(session.state() == initial && resource.live == liveBefore);

    // Tick intentionally commits the timeout before a stale command is rejected,
    // even if the next allocation is armed to fail. Neither operation allocates.
    resource.failNext();
    const auto attempts = resource.attempts;
    clock = 1000;
    assert(session.submit(request("E2", "E4")) == Status::StaleResult);
    const auto skipped = session.state();
    assert(resource.attempts == attempts && skipped.historyCount == 0);
    assert(skipped.revision == initial.revision + 1 && skipped.position.currentTurn == Color::Black);
    auto hashes = session.snapshot(std::pmr::new_delete_resource());
    assert(std::get<SessionSnapshot>(hashes).hashes.back() == skipped.position.hash);
    assert(session.undo() == Status::Unavailable);
    assert(resource.attempts == attempts);
}
