#include "../src/ai/internal.h"
#include <stdio.h>
#include <time.h>
static int64_t clock_ms(void *unused) {
    (void)unused;
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    return (int64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}
int main(void) {
    AcPosition p;
    ac_position_init(&p);
    AcSearchContext *search = ac_search_create();
    if (!search)
        return 1;
    AcSearchOptions options = {
        {clock_ms, NULL},
        10000, 3, NULL, NULL, NULL, 0
    };
    AcSearchResult result;
    AcStatus status = ac_search(search, &p, &options, &result);
    printf("fixture=initial max_depth=3 status=%d completed_depth=%d nodes=%d elapsed_ms=%d "
           "context_allocated_bytes=%zu position_bytes=%zu\n",
           status, result.completedDepth, result.nodes, result.elapsedMs,
           sizeof(*search) + AC_TT_SIZE * sizeof(TTEntry) + (AI_MAX_PLY + 1) * sizeof(AcMoveList), sizeof(p));
    ac_search_destroy(search);
    return status ? 1 : 0;
}
