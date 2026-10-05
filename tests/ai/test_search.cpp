#include "anteater/policy.hpp"
#include "anteater/ai.hpp"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

using namespace ac;
typedef struct {
    int64_t now;
    int ticks, stop;
} Env;
static int64_t now(void *p) {
    Env *e = static_cast<Env *>(p);
    return e->now + (e->ticks++ / 100);
}
static int cancelled(void *p) {
    return ((Env *)p)->stop;
}
int main(void) {
    Position p, before;
    position_init(&p);
    before = p;
    SearchContext *a = search_create(), *b = search_create();
    assert(a && b);
    Env e = {};
    SearchOptions o = {
        {now, &e},
        1000, 2, cancelled, &e, NULL, 0
    };
    SearchResult ra, rb;
    assert(!search(a, &p, &o, &ra));
    assert(validate_move(&p, ra.move));
    assert((p == before));
    e = Env{};
    assert(!search(b, &p, &o, &rb));
    assert((ra.move == rb.move));
    e.stop = 1;
    assert(search(a, &p, &o, &ra) == Status::Cancelled);
    assert((p == before));
    e = Env{};
    o.budgetMs = 1;
    o.maxDepth = 24;
    assert(!search(a, &p, &o, &ra));
    assert(validate_move(&p, ra.move));
    assert(search(a, NULL, &o, &ra) == Status::InvalidArgument);
    TournamentBudget t;
    initialize_tournament_budget(&t);
    int n = tournament_budget_ms(&t, Color::White);
    assert(n > 0);
    charge_tournament_budget(&t, Color::White, n, n - 100);
    assert(t.poolMs[enum_index(Color::White)] >= 100 && t.poolMs[enum_index(Color::Black)] == 0);
    charge_tournament_budget(&t, Color::White, n, 1000000);
    assert(tournament_expired(&t, Color::White));
    search_destroy(a);
    search_destroy(b);
    return 0;
}
