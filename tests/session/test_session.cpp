#include "anteater/session.hpp"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

using namespace ac;
typedef struct {
    int64_t ms;
    int writes, fail, allocs, live, failAt;
} Env;
static int64_t clock_now(void *p) {
    return ((Env *)p)->ms;
}
static Status log_write(void *p, const Snapshot *s) {
    Env *e = static_cast<Env *>(p);
    (void)s;
    ++e->writes;
    return e->fail ? Status::IoError : Status::Ok;
}
static void *alloc(void *p, size_t n) {
    Env *e = static_cast<Env *>(p);
    if (++e->allocs == e->failAt)
        return NULL;
    ++e->live;
    return malloc(n);
}
static void dealloc(void *p, void *x) {
    Env *e = static_cast<Env *>(p);
    --e->live;
    free(x);
}
static Session *create(Env *e) {
    SessionOptions o = {
        {clock_now, e},
        log_write, e, alloc, dealloc, e
    };
    return session_create(&o);
}
static Status move(Session *s, const char *a, const char *b) {
    MoveRequest r;
    assert(!parse_move_request_fields(a, b, PromotionChoice::Queen, &r));
    return session_submit(s, r);
}
int main(void) {
    for (int i = 1; i <= 4; ++i) {
        Env e = {};
        e.failAt = i;
        assert(!create(&e));
        assert(!e.live);
    }
    Env a = {}, b = {};
    Session *x = create(&a), *y = create(&b);
    assert(x && y);
    GameConfig config;
    init_default_game_config(&config);
    config.timerEnabled = 1;
    config.initialTimeSeconds = 2;
    assert(!session_start(x, &config));
    assert(!session_start(y, &config));
    Snapshot sx, sy;
    assert(!move(x, "E2", "E4"));
    session_snapshot(x, &sx);
    session_snapshot(y, &sy);
    assert(sx.historyCount == 1 && sy.historyCount == 0 && a.writes == 2 && b.writes == 1);
    Position saved = sx.position;
    assert(move(x, "A1", "J8") == Status::IllegalMove);
    session_snapshot(x, &sx);
    assert((saved == sx.position));
    assert(!move(x, "E7", "E5"));
    assert(!session_undo(x));
    session_snapshot(x, &sx);
    assert(sx.historyCount == 0 && sx.position.currentTurn == Color::White);
    a.ms = 2000;
    assert(move(x, "E2", "E4") == Status::StaleResult);
    session_snapshot(x, &sx);
    session_snapshot(y, &sy);
    assert(sx.position.currentTurn == Color::Black && sx.historyCount == 0 && sy.position.currentTurn == Color::White &&
           sx.remaining[enum_index(Color::Black)] == 2);
    a.fail = 1;
    assert(!move(x, "E7", "E5"));
    session_snapshot(x, &sx);
    assert(sx.historyCount == 1 && sx.diagnostic == Status::IoError);
    assert(!session_finish(x));
    session_snapshot(x, &sx);
    int64_t elapsed = sx.elapsedMs;
    a.ms += 10000;
    session_snapshot(x, &sx);
    assert(sx.elapsedMs == elapsed && sx.result == GameResult::TerminatedByUser);
    config.mode = GameMode::ComputerVsComputer;
    config.timerEnabled = 0;
    config.aiDifficultyWhite = config.aiDifficultyBlack = Difficulty::Easy;
    assert(!session_start(x, &config));
    session_snapshot(x, &sx);
    MoveList *list = static_cast<MoveList *>(malloc(sizeof(*list)));
    assert(list);
    assert(!generate_legal_moves(&sx.position, list));
    uint64_t revision = sx.revision;
    assert(!session_start(x, &config));
    assert(session_submit_ai(x, list->moves[0], revision, 350, 10) == Status::StaleResult);
    const char *from[] = {"B1", "B8", "C3", "C6"}, *to[] = {"C3", "C6", "B1", "B8"};
    for (int i = 0; i < 8; ++i) {
        session_snapshot(x, &sx);
        MoveRequest r;
        Move m;
        assert(!parse_move_request_fields(from[i % 4], to[i % 4], PromotionChoice::Queen, &r));
        assert(!resolve_move_request(&sx.position, r, &m));
        assert(!session_submit_ai(x, m, sx.revision, 350, 1));
    }
    session_snapshot(x, &sx);
    assert(sx.phase == SessionPhase::Finished && sx.result == GameResult::Draw);
    config.aiDifficultyWhite = Difficulty::Tournament;
    assert(!session_start(x, &config));
    session_snapshot(x, &sx);
    assert(!generate_legal_moves(&sx.position, list));
    assert(!session_submit_ai(x, list->moves[0], sx.revision, 7000, 700000));
    session_snapshot(x, &sx);
    assert(sx.result == GameResult::BlackWin);
    init_default_game_config(&config);
    assert(!session_start(x, &config));
    for (int i = 0; i < MaxMoves; ++i)
        assert(!move(x, from[i % 4], to[i % 4]));
    session_snapshot(x, &sx);
    assert(sx.historyCount == MaxMoves && sx.phase == SessionPhase::Active);
    assert(!move(x, "B1", "C3"));
    session_snapshot(x, &sx);
    assert(sx.result == GameResult::Draw && sx.historyCount == MaxMoves);
    for (int color = value(Color::White); color <= value(Color::Black); ++color) {
        init_game_config_for_mode(&config, GameMode::HumanVsComputer);
        config.playerColor = static_cast<Color>(color);
        assert(!session_start(x, &config));
        for (int ply = 0; ply < 4; ++ply) {
            session_snapshot(x, &sx);
            MoveRequest request;
            Move m;
            assert(!parse_move_request_fields(from[ply], to[ply], PromotionChoice::None, &request));
            if (sx.position.currentTurn == (Color)color)
                assert(!session_submit(x, request));
            else {
                assert(!resolve_move_request(&sx.position, request, &m));
                assert(!session_submit_ai(x, m, sx.revision, 350, 1));
            }
        }
        assert(!session_undo(x));
        session_snapshot(x, &sx);
        assert(sx.position.currentTurn == (Color)color);
        assert(sx.historyCount == (color == value(Color::White) ? 2 : 3));
    }
    free(list);
    session_destroy(x);
    session_destroy(y);
    assert(!a.live && !b.live);
    return 0;
}
