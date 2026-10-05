#ifndef ANTEATER_AI_H
#define ANTEATER_AI_H
#include "anteater/rules.hpp"

namespace ac {
struct SearchContext;
struct SearchOptions {
    Clock clock;
    int budgetMs;
    int maxDepth;
    int (*cancelled)(void *context);
    void *cancelContext;
    const uint64_t *hashes; /* count includes the root position */
    int hashCount;
};
struct SearchResult {
    Move move;
    int nodes, completedDepth, elapsedMs;
    Status status;
};
SearchContext *search_create(void);
void search_destroy(SearchContext *context);
Status search(SearchContext *context, const Position *position, const SearchOptions *options, SearchResult *result);

} // namespace ac
#endif
