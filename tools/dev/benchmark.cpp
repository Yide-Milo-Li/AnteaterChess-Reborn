#include "../../src/ai/internal.hpp"
#include <stdio.h>
#include <time.h>

using namespace ac;
static int64_t clock_ms(void *unused) {
    (void)unused;
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    return (int64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}
int main(void) {
    Position p;
    position_init(&p);
    auto owner = SearchContext::create();
    auto request = SearchRequest::create(p, {}, {10000, 3}, Clock{clock_ms, nullptr});
    if (!std::holds_alternative<SearchContext>(owner) || !std::holds_alternative<SearchRequest>(request))
        return 1;
    auto &context = std::get<SearchContext>(owner);
    auto outcome = context.search(std::get<SearchRequest>(request));
    if (auto *error = std::get_if<Error>(&outcome))
        return int(error->status);
    const auto result = std::get<SearchResult>(outcome);
    const auto status = result.status;
    printf("fixture=initial max_depth=3 status=%d completed_depth=%d nodes=%d elapsed_ms=%d "
           "context_allocated_bytes=%zu position_bytes=%zu\n",
           value(status), result.completedDepth, result.nodes, result.elapsedMs, context.allocated_bytes(), sizeof(p));
    return status != Status::Ok ? 1 : 0;
}
