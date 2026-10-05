#include "anteater/session.hpp"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

using namespace ac;
#include "../core/failing_resource.hpp"
struct Env {
    int64_t ms = 0;
    FailingResource resource;
};
static int64_t clock_now(void *p) {
    return static_cast<Env *>(p)->ms;
}
static Result<Session> create(Env *e) {
    return Session::create(SessionOptions{
        {clock_now, e},
        &e->resource
    });
}
static Status move(Session *s, const char *a, const char *b) {
    MoveRequest r;
    assert(!parse_move_request_fields(a, b, PromotionChoice::Queen, &r));
    return s->submit(r);
}
int main(void) {
    for (int i = 1; i <= 4; ++i) {
        Env e{};
        e.resource.failAt = i;
        auto failed = create(&e);
        assert(std::holds_alternative<Error>(failed));
        assert(e.resource.live == 0);
    }
    Env a{}, b{};
    {
        auto ownerX = create(&a), ownerY = create(&b);
        Session *x = std::get_if<Session>(&ownerX);
        Session *y = std::get_if<Session>(&ownerY);
        assert(x && y);
        GameConfig config;
        init_default_game_config(&config);
        config.timerEnabled = 1;
        config.initialTimeSeconds = 2;
        assert(!x->start(config));
        assert(!y->start(config));
        SessionState sx, sy;
        assert(!move(x, "E2", "E4"));
        sx = x->state();
        sy = y->state();
        assert(sx.historyCount == 1 && sy.historyCount == 0);
        Position saved = sx.position;
        assert(move(x, "A1", "J8") == Status::IllegalMove);
        sx = x->state();
        assert((saved == sx.position));
        assert(!move(x, "E7", "E5"));
        assert(!x->undo());
        sx = x->state();
        assert(sx.historyCount == 0 && sx.position.currentTurn == Color::White);
        a.ms = 2000;
        assert(move(x, "E2", "E4") == Status::StaleResult);
        sx = x->state();
        sy = y->state();
        assert(sx.position.currentTurn == Color::Black && sx.historyCount == 0 &&
               sy.position.currentTurn == Color::White && sx.remaining[enum_index(Color::Black)] == 2);
        assert(!move(x, "E7", "E5"));
        sx = x->state();
        assert(sx.historyCount == 1);
        assert(!x->finish());
        sx = x->state();
        int64_t elapsed = sx.elapsedMs;
        a.ms += 10000;
        sx = x->state();
        assert(sx.elapsedMs == elapsed && sx.result == GameResult::TerminatedByUser);
        config.mode = GameMode::ComputerVsComputer;
        config.timerEnabled = 0;
        config.aiDifficultyWhite = config.aiDifficultyBlack = Difficulty::Easy;
        assert(!x->start(config));
        sx = x->state();
        MoveList *list = static_cast<MoveList *>(malloc(sizeof(*list)));
        assert(list);
        assert(!generate_legal_moves(&sx.position, list));
        uint64_t revision = sx.revision;
        assert(!x->start(config));
        assert(x->submit_ai(list->moves[0], revision, 350, 10) == Status::StaleResult);
        const char *from[] = {"B1", "B8", "C3", "C6"}, *to[] = {"C3", "C6", "B1", "B8"};
        for (int i = 0; i < 8; ++i) {
            sx = x->state();
            MoveRequest r;
            Move m;
            assert(!parse_move_request_fields(from[i % 4], to[i % 4], PromotionChoice::Queen, &r));
            assert(!resolve_move_request(&sx.position, r, &m));
            assert(!x->submit_ai(m, sx.revision, 350, 1));
        }
        sx = x->state();
        assert(sx.phase == SessionPhase::Finished && sx.result == GameResult::Draw);
        config.aiDifficultyWhite = Difficulty::Tournament;
        assert(!x->start(config));
        sx = x->state();
        assert(!generate_legal_moves(&sx.position, list));
        assert(!x->submit_ai(list->moves[0], sx.revision, 7000, 700000));
        sx = x->state();
        assert(sx.result == GameResult::BlackWin);
        init_default_game_config(&config);
        assert(!x->start(config));
        for (int i = 0; i < MaxMoves; ++i)
            assert(!move(x, from[i % 4], to[i % 4]));
        sx = x->state();
        assert(sx.historyCount == MaxMoves && sx.phase == SessionPhase::Active);
        assert(!move(x, "B1", "C3"));
        sx = x->state();
        assert(sx.result == GameResult::Draw && sx.historyCount == MaxMoves);
        for (int color = value(Color::White); color <= value(Color::Black); ++color) {
            init_game_config_for_mode(&config, GameMode::HumanVsComputer);
            config.playerColor = static_cast<Color>(color);
            assert(!x->start(config));
            for (int ply = 0; ply < 4; ++ply) {
                sx = x->state();
                MoveRequest request;
                Move m;
                assert(!parse_move_request_fields(from[ply], to[ply], PromotionChoice::None, &request));
                if (sx.position.currentTurn == (Color)color)
                    assert(!x->submit(request));
                else {
                    assert(!resolve_move_request(&sx.position, request, &m));
                    assert(!x->submit_ai(m, sx.revision, 350, 1));
                }
            }
            assert(!x->undo());
            sx = x->state();
            assert(sx.position.currentTurn == (Color)color);
            assert(sx.historyCount == (color == value(Color::White) ? 2 : 3));
        }
        free(list);
    }
    assert(a.resource.live == 0 && b.resource.live == 0);
    return 0;
}
