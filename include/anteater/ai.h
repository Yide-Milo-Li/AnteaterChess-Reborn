#ifndef ANTEATER_AI_H
#define ANTEATER_AI_H
#include "anteater/rules.h"
typedef struct {
    int remainingMs[2], poolMs[2];
} AcAITimeManager;
void ac_init_ai_time_manager(AcAITimeManager *manager);
int ac_get_ai_tournament_budget_ms(AcAITimeManager *manager, AcColor color);
int ac_is_ai_tournament_time_expired(const AcAITimeManager *manager, AcColor color);
void ac_update_ai_tournament_time(AcAITimeManager *manager, AcColor color, int budgetMs, int elapsedMs);
typedef struct AcSearchContext AcSearchContext;
typedef struct {
    AcClock clock;
    int budgetMs;
    int maxDepth;
    int (*cancelled)(void *context);
    void *cancelContext;
    const uint64_t *hashes; /* count includes the root position */
    int hashCount;
} AcSearchOptions;
typedef struct {
    AcMove move;
    int nodes, completedDepth, elapsedMs;
    AcStatus status;
} AcSearchResult;
AcSearchContext *ac_search_create(void);
void ac_search_destroy(AcSearchContext *context);
AcStatus ac_search(AcSearchContext *context, const AcPosition *position, const AcSearchOptions *options,
                   AcSearchResult *result);
int ac_search_depth(AcAIDifficulty difficulty);
#endif
