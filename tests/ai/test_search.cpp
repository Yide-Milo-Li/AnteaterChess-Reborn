#include "anteater/ai.hpp"
#include <cassert>
using namespace ac;
struct Env {
    int64_t now = 0;
    int ticks = 0;
    std::stop_source *stop = nullptr;
    int stopAt = 0;
};
static int64_t now(void *context) {
    auto &env = *static_cast<Env *>(context);
    if (env.stop && env.ticks >= env.stopAt)
        env.stop->request_stop();
    return env.now + env.ticks++ / 100;
}
static Result<SearchResult> run(SearchContext &context, const Position &position, Env &env, SearchLimits limits,
                                std::stop_token stop = {}) {
    auto request = SearchRequest::create(position, {}, limits, Clock{now, &env}, stop);
    assert(std::holds_alternative<SearchRequest>(request));
    return context.search(std::get<SearchRequest>(request));
}
int main() {
    Position position{};
    position_init(&position);
    const auto before = position;
    auto aOwner = SearchContext::create(), bOwner = SearchContext::create();
    auto &a = std::get<SearchContext>(aOwner);
    auto &b = std::get<SearchContext>(bOwner);
    Env env{};
    auto first = run(a, position, env, {1000, 2});
    const auto ra = std::get<SearchResult>(first);
    assert(validate_move(&position, ra.move) && position == before);
    env = {};
    auto second = run(b, position, env, {1000, 2});
    const auto rb = std::get<SearchResult>(second);
    assert(ra.move == rb.move && ra.nodes == rb.nodes && ra.completedDepth == rb.completedDepth);
    std::stop_source stop;
    stop.request_stop();
    assert(std::get<Error>(run(a, position, env, {1000, 2}, stop.get_token())).status == Status::Cancelled);
    assert(position == before);
    std::stop_source during;
    env = {0, 0, &during, 2};
    assert(std::get<Error>(run(a, position, env, {1000, 24}, during.get_token())).status == Status::Cancelled);
    assert(position == before);
    env = {};
    auto timed = run(a, position, env, {1, 24});
    assert(validate_move(&position, std::get<SearchResult>(timed).move));
    assert(std::get<Error>(SearchRequest::create(position, {}, {1000, 2}, {})).status == Status::InvalidArgument);
}
