#ifndef ANTEATER_AI_H
#define ANTEATER_AI_H
#include "anteater/rules.hpp"

namespace ac {
struct AITimeManager {
    int remainingMs[2], poolMs[2];
};
void init_ai_time_manager(AITimeManager *manager);
int get_ai_tournament_budget_ms(AITimeManager *manager, Color color);
int is_ai_tournament_time_expired(const AITimeManager *manager, Color color);
void update_ai_tournament_time(AITimeManager *manager, Color color, int budgetMs, int elapsedMs);
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
int search_depth(Difficulty difficulty);

} // namespace ac
#endif
