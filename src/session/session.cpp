#include "anteater/session.hpp"
#include "anteater/ai.hpp"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

namespace ac {

struct Session {
    SessionOptions options;
    Position position;
    GameConfig config;
    SessionPhase phase;
    GameResult result;
    Status diagnostic;
    uint64_t revision;
    uint64_t gameId;
    Move *moves;
    Undo *undo;
    uint64_t *hashes;
    int count;
    int64_t startMs, turnMs, finishedMs;
    AITimeManager tournament;
};
static void *default_allocate(void *context, size_t n) {
    (void)context;
    return malloc(n);
}
static void default_free(void *context, void *p) {
    (void)context;
    free(p);
}
static int64_t now(const Session *s) {
    return s->options.clock.now(s->options.clock.context);
}
static int64_t nonnegative_delta(int64_t a, int64_t b) {
    return a > b ? a - b : 0;
}
int session_is_ai(const GameConfig *c, Color color) {
    return c && (c->mode == GameMode::ComputerVsComputer ||
                 (c->mode == GameMode::HumanVsComputer && color != c->playerColor));
}
Session *session_create(const SessionOptions *options) {
    if (!options || !options->clock.now || (!!options->allocate != !!options->deallocate))
        return NULL;
    SessionOptions o = *options;
    if (!o.allocate) {
        o.allocate = default_allocate;
        o.deallocate = default_free;
    }
    Session *s = static_cast<Session *>(o.allocate(o.allocatorContext, sizeof(*s)));
    if (!s)
        return NULL;
    memset(s, 0, sizeof(*s));
    s->options = o;
    s->moves = static_cast<Move *>(o.allocate(o.allocatorContext, MaxMoves * sizeof(*s->moves)));
    s->undo = static_cast<Undo *>(o.allocate(o.allocatorContext, MaxMoves * sizeof(*s->undo)));
    s->hashes = static_cast<uint64_t *>(o.allocate(o.allocatorContext, (MaxMoves + 1) * sizeof(*s->hashes)));
    if (!s->moves || !s->undo || !s->hashes) {
        session_destroy(s);
        return NULL;
    }
    position_init(&s->position);
    init_default_game_config(&s->config);
    s->hashes[0] = s->position.hash;
    init_ai_time_manager(&s->tournament);
    return s;
}
void session_destroy(Session *s) {
    if (!s)
        return;
    SessionOptions o = s->options;
    if (s->moves)
        o.deallocate(o.allocatorContext, s->moves);
    if (s->undo)
        o.deallocate(o.allocatorContext, s->undo);
    if (s->hashes)
        o.deallocate(o.allocatorContext, s->hashes);
    o.deallocate(o.allocatorContext, s);
}
Status session_snapshot(const Session *s, Snapshot *out) {
    if (!s || !out)
        return Status::InvalidArgument;
    memset(out, 0, sizeof(*out));
    out->position = s->position;
    out->config = s->config;
    out->phase = s->phase;
    out->result = s->result;
    out->revision = s->revision;
    out->gameId = s->gameId;
    out->history = s->moves;
    out->historyCount = s->count;
    out->hashes = s->hashes;
    out->diagnostic = s->diagnostic;
    int64_t at = s->phase == SessionPhase::Finished ? s->finishedMs : now(s);
    out->elapsedMs = s->phase == SessionPhase::Idle ? 0 : nonnegative_delta(at, s->startMs);
    for (int c = 0; c < 2; ++c) {
        int64_t remaining = s->config.initialTimeSeconds;
        if (s->config.timerEnabled && c == (int)s->position.currentTurn)
            remaining -= nonnegative_delta(at, s->turnMs) / 1000;
        out->remaining[c] = remaining < 0 ? 0 : remaining > INT_MAX ? INT_MAX : (int)remaining;
        out->tournamentRemainingMs[c] = s->tournament.remainingMs[c];
    }
    return Status::Ok;
}
static void publish(Session *s) {
    ++s->revision;
    if (s->options.log) {
        Snapshot snap;
        session_snapshot(s, &snap);
        s->diagnostic = s->options.log(s->options.logContext, &snap);
    }
}
static void end(Session *s, GameResult result) {
    s->phase = SessionPhase::Finished;
    s->result = result;
    s->finishedMs = now(s);
}
static int valid_difficulty(Difficulty difficulty) {
    /* Enum membership matters now that the removed alias leaves a numeric hole. */
    switch (difficulty) {
    case Difficulty::None:
    case Difficulty::Easy:
    case Difficulty::Medium:
    case Difficulty::Hard:
    case Difficulty::Tournament:
        return 1;
    default:
        return 0;
    }
}
Status session_start(Session *s, const GameConfig *c) {
    if (!s || !c || c->mode < GameMode::HumanVsHuman || c->mode > GameMode::ComputerVsComputer ||
        !valid_difficulty(c->aiDifficultyWhite) || !valid_difficulty(c->aiDifficultyBlack) ||
        (c->mode == GameMode::HumanVsComputer && c->playerColor != Color::White && c->playerColor != Color::Black) ||
        c->initialTimeSeconds < 0 || c->aiTimeLimit < 0 || (c->timerEnabled && c->initialTimeSeconds <= 0) ||
        !is_ai_turn_timer_setting_valid(c))
        return Status::InvalidArgument;
    position_init(&s->position);
    s->config = *c;
    s->count = 0;
    s->hashes[0] = s->position.hash;
    s->phase = SessionPhase::Active;
    ++s->gameId;
    s->result = GameResult::None;
    s->diagnostic = Status::Ok;
    s->startMs = s->turnMs = now(s);
    init_ai_time_manager(&s->tournament);
    publish(s);
    return Status::Ok;
}
Status session_tick(Session *s) {
    if (!s)
        return Status::InvalidArgument;
    if (s->phase != SessionPhase::Active || !s->config.timerEnabled)
        return Status::Ok;
    int64_t at = now(s);
    if (nonnegative_delta(at, s->turnMs) / 1000 >= s->config.initialTimeSeconds) {
        s->position.currentTurn = s->position.currentTurn == Color::White ? Color::Black : Color::White;
        s->position.hash = position_hash(&s->position);
        s->hashes[s->count] = s->position.hash;
        s->turnMs = at;
        publish(s);
    }
    return Status::Ok;
}
static Status commit_move(Session *s, Move move) {
    if (s->count == MaxMoves) {
        end(s, GameResult::Draw);
        publish(s);
        return Status::Ok;
    }
    Position next = s->position;
    Undo undo;
    GameResult result;
    Status status = position_apply(&next, move, &undo);
    if (status != Status::Ok)
        return status;
    status = position_result(&next, &result);
    if (status != Status::Ok)
        return status;
    if (result == GameResult::None && s->config.mode == GameMode::ComputerVsComputer) {
        int repetitions = 1;
        for (int i = 0; i <= s->count; ++i)
            if (s->hashes[i] == next.hash)
                ++repetitions;
        if (repetitions >= 3)
            result = GameResult::Draw;
    }
    s->moves[s->count] = move;
    s->undo[s->count] = undo;
    ++s->count;
    s->position = next;
    s->hashes[s->count] = next.hash;
    s->turnMs = now(s);
    if (result != GameResult::None)
        end(s, result);
    publish(s);
    return Status::Ok;
}
Status session_submit(Session *s, MoveRequest request) {
    if (!s)
        return Status::InvalidArgument;
    uint64_t revision = s->revision;
    session_tick(s);
    if (s->revision != revision)
        return Status::StaleResult;
    if (s->phase != SessionPhase::Active || session_is_ai(&s->config, s->position.currentTurn))
        return Status::Unavailable;
    Move move;
    Status resolved = resolve_move_request(&s->position, request, &move);
    if (resolved != Status::Ok)
        return resolved == Status::OutOfMemory || resolved == Status::Capacity ? static_cast<Status>(resolved)
                                                                               : Status::IllegalMove;
    return commit_move(s, move);
}
Status session_submit_ai(Session *s, Move move, uint64_t revision, int budgetMs, int elapsedMs) {
    if (!s || budgetMs <= 0 || elapsedMs < 0)
        return Status::InvalidArgument;
    session_tick(s);
    if (revision != s->revision)
        return Status::StaleResult;
    if (s->phase != SessionPhase::Active || !session_is_ai(&s->config, s->position.currentTurn))
        return Status::Unavailable;
    Color color = s->position.currentTurn;
    Difficulty d = color == Color::White ? s->config.aiDifficultyWhite : s->config.aiDifficultyBlack;
    if (d == Difficulty::Tournament) {
        if (elapsedMs > s->tournament.remainingMs[enum_index(color)] ||
            s->tournament.remainingMs[enum_index(color)] <= 0) {
            end(s, color == Color::White ? GameResult::BlackWin : GameResult::WhiteWin);
            publish(s);
            return Status::Ok;
        }
        AITimeManager previous = s->tournament;
        update_ai_tournament_time(&s->tournament, color, budgetMs, elapsedMs);
        Status status = commit_move(s, move);
        if (status != Status::Ok)
            s->tournament = previous;
        return status;
    }
    return commit_move(s, move);
}
Status session_undo(Session *s) {
    if (!s)
        return Status::InvalidArgument;
    uint64_t revision = s->revision;
    session_tick(s);
    if (revision != s->revision)
        return Status::StaleResult;
    if (s->phase != SessionPhase::Active || s->count == 0)
        return Status::Unavailable;
    int target = s->count - 1;
    if (s->config.mode == GameMode::HumanVsHuman)
        target = s->count >= 2 ? s->count - 2 : 0;
    else if (s->config.mode == GameMode::HumanVsComputer) {
        while (target >= 0 && ((target % 2) == 0 ? Color::White : Color::Black) != s->config.playerColor)
            --target;
        if (target < 0)
            return Status::Unavailable;
    }
    while (s->count > target) {
        --s->count;
        position_unmake(&s->position, &s->undo[s->count]);
    }
    s->turnMs = now(s);
    s->hashes[s->count] = s->position.hash;
    publish(s);
    return Status::Ok;
}
Status session_finish(Session *s) {
    if (!s)
        return Status::InvalidArgument;
    if (s->phase == SessionPhase::Active) {
        end(s, GameResult::TerminatedByUser);
        publish(s);
    }
    return Status::Ok;
}
Status session_promotion(const Session *s, MoveRequest r, int *needed) {
    if (!s || !needed)
        return Status::InvalidArgument;
    *needed = 0;
    if (s->phase != SessionPhase::Active)
        return Status::Unavailable;
    Move m;
    if (resolve_move_request(&s->position, r, &m) != Status::Ok)
        return Status::IllegalMove;
    *needed = is_promotion_special_move(m.specialType);
    return Status::Ok;
}
int session_ai_budget(const Session *s) {
    if (!s || s->phase != SessionPhase::Active)
        return 0;
    Color c = s->position.currentTurn;
    Difficulty d = c == Color::White ? s->config.aiDifficultyWhite : s->config.aiDifficultyBlack;
    if (d == Difficulty::Tournament) {
        AITimeManager copy = s->tournament;
        return get_ai_tournament_budget_ms(&copy, c);
    }
    return get_ai_time_budget_ms(&s->config, d);
}
const char *status_message(Status status) {
    switch (status) {
    case Status::Ok:
        return "Ready";
    case Status::InvalidArgument:
        return "Invalid input or configuration";
    case Status::IllegalMove:
        return "Illegal or ambiguous move";
    case Status::OutOfMemory:
        return "Insufficient memory";
    case Status::Capacity:
        return "Move buffer capacity exceeded";
    case Status::Cancelled:
        return "Search cancelled";
    case Status::StaleResult:
        return "The position changed; please try again";
    case Status::Unavailable:
        return "Action unavailable";
    case Status::IoError:
        return "The game continues, but its log could not be written";
    default:
        return "Unknown error";
    }
}

} // namespace ac
