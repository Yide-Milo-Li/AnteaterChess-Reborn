#include "anteater/session.hpp"
#include "../core/failing_resource.hpp"
#include <cassert>
#include <optional>
#include <type_traits>

using namespace ac;
static int64_t clockNow(void *context) {
    return *static_cast<int64_t *>(context);
}
static Session makeSession(FailingResource &resource, int64_t &clock) {
    auto result = Session::create(SessionOptions{
        {clockNow, &clock},
        &resource
    });
    assert(std::holds_alternative<Session>(result));
    return std::move(std::get<Session>(result));
}
static GameConfig config() {
    GameConfig value{};
    init_default_game_config(&value);
    return value;
}
static MoveRequest request(const char *from, const char *to) {
    MoveRequest move{};
    assert(parse_move_request_fields(from, to, PromotionChoice::None, &move) == Status::Ok);
    return move;
}
int main() {
    static_assert(!std::is_copy_constructible_v<Session>);
    static_assert(std::is_nothrow_move_constructible_v<Session>);
    static_assert(std::is_nothrow_move_assignable_v<Session>);
    int64_t clock = 0;
    bool completed = false;
    for (std::size_t failure = 1; failure < 64 && !completed; ++failure) {
        FailingResource resource;
        resource.failAt = failure;
        {
            auto result = Session::create(SessionOptions{
                {clockNow, &clock},
                &resource
            });
            if (auto *error = std::get_if<Error>(&result)) {
                assert(error->status == Status::OutOfMemory && resource.live == 0);
            } else
                completed = true;
        }
        assert(resource.live == 0);
    }
    assert(completed);

    FailingResource resource;
    std::optional<SessionSnapshot> survivor;
    Position saved{};
    {
        auto session = makeSession(resource, clock);
        assert(session.start(config()) == Status::Ok);
        const auto readsBefore = resource.attempts;
        for (int i = 0; i < 100; ++i)
            (void)session.state();
        assert(resource.attempts == readsBefore);
        assert(session.submit(request("E2", "E4")) == Status::Ok);
        saved = session.state().position;
        const auto liveBefore = resource.live;
        completed = false;
        for (std::size_t failure = 1; failure < 64 && !completed; ++failure) {
            resource.failAt = resource.attempts + failure;
            auto snapshot = session.snapshot();
            if (auto *error = std::get_if<Error>(&snapshot)) {
                assert(error->status == Status::OutOfMemory && resource.live == liveBefore);
                assert(session.state().position == saved && session.state().historyCount == 1);
            } else {
                completed = true;
                survivor.emplace(std::move(std::get<SessionSnapshot>(snapshot)));
            }
        }
        assert(completed);
        resource.failAt = 0;
        assert(session.submit(request("E7", "E5")) == Status::Ok);
        assert(session.undo() == Status::Ok);
        assert(session.start(config()) == Status::Ok);
        assert(survivor->position == saved && survivor->history.size() == 1);
        Session moved = std::move(session);
        assert(!session.valid() && moved.valid());
        auto replacement = makeSession(resource, clock);
        const auto attempts = resource.attempts;
        replacement = std::move(moved);
        assert(resource.attempts == attempts && !moved.valid());
    }
    assert(survivor->position == saved && survivor->history.size() == 1 && survivor->hashes.size() == 2);
    assert(survivor->hashes.back() == saved.hash);
    FailingResource otherResource;
    {
        SessionSnapshot destination{&otherResource};
        const std::array<uint64_t, 1> prior{42};
        destination.hashes.assign(prior.begin(), prior.end());
        const auto attempts = resource.attempts + otherResource.attempts;
        destination = std::move(*survivor);
        survivor.reset();
        assert(resource.attempts + otherResource.attempts == attempts);
        assert(otherResource.live == 0 && destination.position == saved);
    }
    assert(resource.live == 0);

    // Every allocation used to prepare a move may fail; none may publish a
    // partial position, revision, history entry or Tournament balance.
    auto session = makeSession(resource, clock);
    assert(session.start(config()) == Status::Ok);
    auto before = session.state();
    const auto liveBefore = resource.live;
    completed = false;
    for (std::size_t failure = 1; failure < 64 && !completed; ++failure) {
        resource.failAt = resource.attempts + failure;
        auto status = session.submit(request("E2", "E4"));
        if (status == Status::Ok)
            completed = true;
        else {
            assert(status == Status::OutOfMemory && session.state() == before);
            assert(resource.live == liveBefore);
        }
    }
    assert(completed);
}
