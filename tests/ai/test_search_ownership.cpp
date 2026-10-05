#include "anteater/ai.hpp"
#include "anteater/session.hpp"
#include "../core/failing_resource.hpp"
#include <cassert>
#include <optional>
#include <type_traits>
using namespace ac;
static int64_t now(void *) {
    return 0;
}
int main() {
    static_assert(!std::is_copy_constructible_v<SearchContext>);
    static_assert(std::is_nothrow_move_constructible_v<SearchContext>);
    static_assert(std::is_nothrow_move_assignable_v<SearchRequest>);
    bool completed = false;
    for (std::size_t failure = 1; failure < 64 && !completed; ++failure) {
        FailingResource resource;
        resource.failAt = failure;
        {
            auto result = SearchContext::create(&resource);
            if (auto *error = std::get_if<Error>(&result))
                assert(error->status == Status::OutOfMemory && resource.live == 0);
            else
                completed = true;
        }
        assert(resource.live == 0);
    }
    assert(completed);
    FailingResource resource;
    std::optional<SearchRequest> survivor;
    Position saved{};
    {
        auto owner = Session::create(SessionOptions{
            {now, nullptr}
        });
        auto &session = std::get<Session>(owner);
        GameConfig config{};
        init_default_game_config(&config);
        assert(session.start(config) == Status::Ok);
        auto snapshot = session.snapshot();
        auto &state = std::get<SessionSnapshot>(snapshot);
        saved = state.position;
        completed = false;
        for (std::size_t failure = 1; failure < 64 && !completed; ++failure) {
            resource.failAt = resource.attempts + failure;
            auto result = SearchRequest::create(state.position, {state.hashes.data(), state.hashes.size()}, {1000, 2},
                                                Clock{now, nullptr}, {}, &resource);
            if (auto *error = std::get_if<Error>(&result))
                assert(error->status == Status::OutOfMemory && resource.live == 0);
            else {
                survivor.emplace(std::move(std::get<SearchRequest>(result)));
                completed = true;
            }
        }
        assert(completed);
        resource.failAt = 0;
        assert(session.start(config) == Status::Ok);
    }
    assert(survivor->position() == saved && survivor->hashes().size() == 1 && survivor->hashes()[0] == saved.hash);
    auto created = SearchContext::create(&resource);
    auto context = std::move(std::get<SearchContext>(created));
    auto moved = std::move(context);
    assert(!context.valid() && moved.valid());
    const auto attempts = resource.attempts;
    auto outcome = moved.search(*survivor);
    assert(std::holds_alternative<SearchResult>(outcome) && resource.attempts == attempts);
    assert(validate_move(&saved, std::get<SearchResult>(outcome).move));
    auto replacement = SearchContext::create(&resource);
    const auto movingAttempts = resource.attempts;
    std::get<SearchContext>(replacement) = std::move(moved);
    assert(!moved.valid() && resource.attempts == movingAttempts);
    assert(std::get<Error>(context.search(*survivor)).status == Status::Unavailable);
    std::stop_source stop;
    auto cancelled = SearchRequest::create(saved, {}, {1000, 2}, Clock{now, nullptr}, stop.get_token(), &resource);
    stop.request_stop();
    assert(std::get<Error>(std::get<SearchContext>(replacement).search(std::get<SearchRequest>(cancelled))).status ==
           Status::Cancelled);
    assert(std::get<Error>(SearchRequest::create(saved, {}, {0, 2}, Clock{now, nullptr})).status ==
           Status::InvalidArgument);
}
