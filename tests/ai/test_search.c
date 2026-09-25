#include "anteater/ai.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
typedef struct {
    int64_t now;
    int ticks, stop;
} Env;
static int64_t now(void *p) {
    Env *e = p;
    return e->now + (e->ticks++ / 100);
}
static int cancelled(void *p) {
    return ((Env *)p)->stop;
}
int main(void) {
    AcPosition p, before;
    ac_position_init(&p);
    before = p;
    AcSearchContext *a = ac_search_create(), *b = ac_search_create();
    assert(a && b);
    Env e = {0};
    AcSearchOptions o = {
        {now, &e},
        1000, 2, cancelled, &e, NULL, 0
    };
    AcSearchResult ra, rb;
    assert(!ac_search(a, &p, &o, &ra));
    assert(ac_validate_move(&p, ra.move));
    assert(!memcmp(&p, &before, sizeof(p)));
    e = (Env){0};
    assert(!ac_search(b, &p, &o, &rb));
    assert(!memcmp(&ra.move, &rb.move, sizeof(AcMove)));
    e.stop = 1;
    assert(ac_search(a, &p, &o, &ra) == AC_CANCELLED);
    assert(!memcmp(&p, &before, sizeof(p)));
    e = (Env){0};
    o.budgetMs = 1;
    o.maxDepth = 24;
    assert(!ac_search(a, &p, &o, &ra));
    assert(ac_validate_move(&p, ra.move));
    assert(ac_search(a, NULL, &o, &ra) == AC_INVALID_ARGUMENT);
    AcAITimeManager t;
    ac_init_ai_time_manager(&t);
    int n = ac_get_ai_tournament_budget_ms(&t, AC_WHITE);
    assert(n > 0);
    ac_update_ai_tournament_time(&t, AC_WHITE, n, n - 100);
    assert(t.poolMs[AC_WHITE] >= 100 && t.poolMs[AC_BLACK] == 0);
    ac_update_ai_tournament_time(&t, AC_WHITE, n, 1000000);
    assert(ac_is_ai_tournament_time_expired(&t, AC_WHITE));
    ac_search_destroy(a);
    ac_search_destroy(b);
    return 0;
}
