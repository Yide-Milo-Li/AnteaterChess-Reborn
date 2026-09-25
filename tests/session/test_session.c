#include "anteater/session.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    int64_t ms;
    int writes, fail, allocs, live, failAt;
} Env;
static int64_t clock_now(void *p) {
    return ((Env *)p)->ms;
}
static AcStatus log_write(void *p, const AcSnapshot *s) {
    Env *e = p;
    (void)s;
    ++e->writes;
    return e->fail ? AC_IO_ERROR : AC_OK;
}
static void *alloc(void *p, size_t n) {
    Env *e = p;
    if (++e->allocs == e->failAt)
        return NULL;
    ++e->live;
    return malloc(n);
}
static void dealloc(void *p, void *x) {
    Env *e = p;
    --e->live;
    free(x);
}
static AcSession *create(Env *e) {
    AcSessionOptions o = {
        {clock_now, e},
        log_write, e, alloc, dealloc, e
    };
    return ac_session_create(&o);
}
static AcStatus move(AcSession *s, const char *a, const char *b) {
    AcMoveRequest r;
    assert(!ac_parse_move_request_fields(a, b, AC_PROMOTION_CHOICE_QUEEN, &r));
    return ac_session_submit(s, r);
}
int main(void) {
    for (int i = 1; i <= 4; ++i) {
        Env e = {0};
        e.failAt = i;
        assert(!create(&e));
        assert(!e.live);
    }
    Env a = {0}, b = {0};
    AcSession *x = create(&a), *y = create(&b);
    assert(x && y);
    AcGameConfig config;
    ac_init_default_game_config(&config);
    config.timerEnabled = 1;
    config.initialTimeSeconds = 2;
    assert(!ac_session_start(x, &config));
    assert(!ac_session_start(y, &config));
    AcSnapshot sx, sy;
    assert(!move(x, "E2", "E4"));
    ac_session_snapshot(x, &sx);
    ac_session_snapshot(y, &sy);
    assert(sx.historyCount == 1 && sy.historyCount == 0 && a.writes == 2 && b.writes == 1);
    AcPosition saved = sx.position;
    assert(move(x, "A1", "J8") == AC_ILLEGAL_MOVE);
    ac_session_snapshot(x, &sx);
    assert(!memcmp(&saved, &sx.position, sizeof(saved)));
    assert(!move(x, "E7", "E5"));
    assert(!ac_session_undo(x));
    ac_session_snapshot(x, &sx);
    assert(sx.historyCount == 0 && sx.position.currentTurn == AC_WHITE);
    a.ms = 2000;
    assert(move(x, "E2", "E4") == AC_STALE_RESULT);
    ac_session_snapshot(x, &sx);
    ac_session_snapshot(y, &sy);
    assert(sx.position.currentTurn == AC_BLACK && sx.historyCount == 0 && sy.position.currentTurn == AC_WHITE &&
           sx.remaining[AC_BLACK] == 2);
    a.fail = 1;
    assert(!move(x, "E7", "E5"));
    ac_session_snapshot(x, &sx);
    assert(sx.historyCount == 1 && sx.diagnostic == AC_IO_ERROR);
    assert(!ac_session_finish(x));
    ac_session_snapshot(x, &sx);
    int64_t elapsed = sx.elapsedMs;
    a.ms += 10000;
    ac_session_snapshot(x, &sx);
    assert(sx.elapsedMs == elapsed && sx.result == AC_RESULT_TERMINATED_BY_USER);
    config.mode = AC_MODE_COMPUTER_VS_COMPUTER;
    config.timerEnabled = 0;
    config.aiDifficultyWhite = config.aiDifficultyBlack = AC_DIFFICULTY_EASY;
    assert(!ac_session_start(x, &config));
    ac_session_snapshot(x, &sx);
    AcMoveList *list = malloc(sizeof(*list));
    assert(list);
    assert(!ac_generate_legal_moves(&sx.position, list));
    uint64_t revision = sx.revision;
    assert(!ac_session_start(x, &config));
    assert(ac_session_submit_ai(x, list->moves[0], revision, 350, 10) == AC_STALE_RESULT);
    const char *from[] = {"B1", "B8", "C3", "C6"}, *to[] = {"C3", "C6", "B1", "B8"};
    for (int i = 0; i < 8; ++i) {
        ac_session_snapshot(x, &sx);
        AcMoveRequest r;
        AcMove m;
        assert(!ac_parse_move_request_fields(from[i % 4], to[i % 4], AC_PROMOTION_CHOICE_QUEEN, &r));
        assert(!ac_resolve_move_request(&sx.position, r, &m));
        assert(!ac_session_submit_ai(x, m, sx.revision, 350, 1));
    }
    ac_session_snapshot(x, &sx);
    assert(sx.phase == AC_SESSION_FINISHED && sx.result == AC_RESULT_DRAW);
    config.aiDifficultyWhite = AC_DIFFICULTY_TOURNAMENT;
    assert(!ac_session_start(x, &config));
    ac_session_snapshot(x, &sx);
    assert(!ac_generate_legal_moves(&sx.position, list));
    assert(!ac_session_submit_ai(x, list->moves[0], sx.revision, 7000, 700000));
    ac_session_snapshot(x, &sx);
    assert(sx.result == AC_RESULT_BLACK_WIN);
    ac_init_default_game_config(&config);
    assert(!ac_session_start(x, &config));
    for (int i = 0; i < AC_MAX_MOVES; ++i)
        assert(!move(x, from[i % 4], to[i % 4]));
    ac_session_snapshot(x, &sx);
    assert(sx.historyCount == AC_MAX_MOVES && sx.phase == AC_SESSION_ACTIVE);
    assert(!move(x, "B1", "C3"));
    ac_session_snapshot(x, &sx);
    assert(sx.result == AC_RESULT_DRAW && sx.historyCount == AC_MAX_MOVES);
    for (int color = AC_WHITE; color <= AC_BLACK; ++color) {
        ac_init_game_config_for_mode(&config, AC_MODE_HUMAN_VS_COMPUTER);
        config.playerColor = color;
        assert(!ac_session_start(x, &config));
        for (int ply = 0; ply < 4; ++ply) {
            ac_session_snapshot(x, &sx);
            AcMoveRequest request;
            AcMove m;
            assert(!ac_parse_move_request_fields(from[ply], to[ply], AC_PROMOTION_CHOICE_NONE, &request));
            if (sx.position.currentTurn == (AcColor)color)
                assert(!ac_session_submit(x, request));
            else {
                assert(!ac_resolve_move_request(&sx.position, request, &m));
                assert(!ac_session_submit_ai(x, m, sx.revision, 350, 1));
            }
        }
        assert(!ac_session_undo(x));
        ac_session_snapshot(x, &sx);
        assert(sx.position.currentTurn == (AcColor)color);
        assert(sx.historyCount == (color == AC_WHITE ? 2 : 3));
    }
    free(list);
    ac_session_destroy(x);
    ac_session_destroy(y);
    assert(!a.live && !b.live);
    return 0;
}
