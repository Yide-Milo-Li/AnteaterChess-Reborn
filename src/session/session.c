#include "anteater/session.h"
#include "anteater/ai.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

struct AcSession {
    AcSessionOptions options;
    AcPosition position;
    AcGameConfig config;
    AcSessionPhase phase;
    AcGameResult result;
    AcStatus diagnostic;
    uint64_t revision;
    uint64_t gameId;
    AcMove *moves;
    AcUndo *undo;
    uint64_t *hashes;
    int count;
    int64_t startMs, turnMs, finishedMs;
    AcAITimeManager tournament;
};
static void *default_allocate(void *context, size_t n) {
    (void)context;
    return malloc(n);
}
static void default_free(void *context, void *p) {
    (void)context;
    free(p);
}
static int64_t now(const AcSession *s) {
    return s->options.clock.now(s->options.clock.context);
}
static int64_t nonnegative_delta(int64_t a, int64_t b) {
    return a > b ? a - b : 0;
}
int ac_session_is_ai(const AcGameConfig *c, AcColor color) {
    return c && (c->mode == AC_MODE_COMPUTER_VS_COMPUTER ||
                 (c->mode == AC_MODE_HUMAN_VS_COMPUTER && color != c->playerColor));
}
AcSession *ac_session_create(const AcSessionOptions *options) {
    if (!options || !options->clock.now || (!!options->allocate != !!options->deallocate))
        return NULL;
    AcSessionOptions o = *options;
    if (!o.allocate) {
        o.allocate = default_allocate;
        o.deallocate = default_free;
    }
    AcSession *s = o.allocate(o.allocatorContext, sizeof(*s));
    if (!s)
        return NULL;
    memset(s, 0, sizeof(*s));
    s->options = o;
    s->moves = o.allocate(o.allocatorContext, AC_MAX_MOVES * sizeof(*s->moves));
    s->undo = o.allocate(o.allocatorContext, AC_MAX_MOVES * sizeof(*s->undo));
    s->hashes = o.allocate(o.allocatorContext, (AC_MAX_MOVES + 1) * sizeof(*s->hashes));
    if (!s->moves || !s->undo || !s->hashes) {
        ac_session_destroy(s);
        return NULL;
    }
    ac_position_init(&s->position);
    ac_init_default_game_config(&s->config);
    s->hashes[0] = s->position.hash;
    ac_init_ai_time_manager(&s->tournament);
    return s;
}
void ac_session_destroy(AcSession *s) {
    if (!s)
        return;
    AcSessionOptions o = s->options;
    if (s->moves)
        o.deallocate(o.allocatorContext, s->moves);
    if (s->undo)
        o.deallocate(o.allocatorContext, s->undo);
    if (s->hashes)
        o.deallocate(o.allocatorContext, s->hashes);
    o.deallocate(o.allocatorContext, s);
}
AcStatus ac_session_snapshot(const AcSession *s, AcSnapshot *out) {
    if (!s || !out)
        return AC_INVALID_ARGUMENT;
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
    int64_t at = s->phase == AC_SESSION_FINISHED ? s->finishedMs : now(s);
    out->elapsedMs = s->phase == AC_SESSION_IDLE ? 0 : nonnegative_delta(at, s->startMs);
    for (int c = 0; c < 2; ++c) {
        int64_t remaining = s->config.initialTimeSeconds;
        if (s->config.timerEnabled && c == (int)s->position.currentTurn)
            remaining -= nonnegative_delta(at, s->turnMs) / 1000;
        out->remaining[c] = remaining < 0 ? 0 : remaining > INT_MAX ? INT_MAX : (int)remaining;
        out->tournamentRemainingMs[c] = s->tournament.remainingMs[c];
    }
    return AC_OK;
}
static void publish(AcSession *s) {
    ++s->revision;
    if (s->options.log) {
        AcSnapshot snap;
        ac_session_snapshot(s, &snap);
        s->diagnostic = s->options.log(s->options.logContext, &snap);
    }
}
static void end(AcSession *s, AcGameResult result) {
    s->phase = AC_SESSION_FINISHED;
    s->result = result;
    s->finishedMs = now(s);
}
AcStatus ac_session_start(AcSession *s, const AcGameConfig *c) {
    if (!s || !c || c->mode < AC_MODE_HUMAN_VS_HUMAN || c->mode > AC_MODE_COMPUTER_VS_COMPUTER ||
        c->aiDifficultyWhite < AC_DIFFICULTY_NONE || c->aiDifficultyWhite > AC_DIFFICULTY_TOURNAMENT ||
        c->aiDifficultyBlack < AC_DIFFICULTY_NONE || c->aiDifficultyBlack > AC_DIFFICULTY_TOURNAMENT ||
        (c->mode == AC_MODE_HUMAN_VS_COMPUTER && c->playerColor != AC_WHITE && c->playerColor != AC_BLACK) ||
        c->initialTimeSeconds < 0 || c->aiTimeLimit < 0 || (c->timerEnabled && c->initialTimeSeconds <= 0) ||
        !ac_is_ai_turn_timer_setting_valid(c))
        return AC_INVALID_ARGUMENT;
    ac_position_init(&s->position);
    s->config = *c;
    s->count = 0;
    s->hashes[0] = s->position.hash;
    s->phase = AC_SESSION_ACTIVE;
    ++s->gameId;
    s->result = AC_RESULT_NONE;
    s->diagnostic = AC_OK;
    s->startMs = s->turnMs = now(s);
    ac_init_ai_time_manager(&s->tournament);
    publish(s);
    return AC_OK;
}
AcStatus ac_session_tick(AcSession *s) {
    if (!s)
        return AC_INVALID_ARGUMENT;
    if (s->phase != AC_SESSION_ACTIVE || !s->config.timerEnabled)
        return AC_OK;
    int64_t at = now(s);
    if (nonnegative_delta(at, s->turnMs) / 1000 >= s->config.initialTimeSeconds) {
        s->position.currentTurn = s->position.currentTurn == AC_WHITE ? AC_BLACK : AC_WHITE;
        s->position.hash = ac_position_hash(&s->position);
        s->hashes[s->count] = s->position.hash;
        s->turnMs = at;
        publish(s);
    }
    return AC_OK;
}
static AcStatus commit_move(AcSession *s, AcMove move) {
    if (s->count == AC_MAX_MOVES) {
        end(s, AC_RESULT_DRAW);
        publish(s);
        return AC_OK;
    }
    AcPosition next = s->position;
    AcUndo undo;
    AcGameResult result;
    AcStatus status = ac_position_apply(&next, move, &undo);
    if (status)
        return status;
    status = ac_position_result(&next, &result);
    if (status)
        return status;
    if (result == AC_RESULT_NONE && s->config.mode == AC_MODE_COMPUTER_VS_COMPUTER) {
        int repetitions = 1;
        for (int i = 0; i <= s->count; ++i)
            if (s->hashes[i] == next.hash)
                ++repetitions;
        if (repetitions >= 3)
            result = AC_RESULT_DRAW;
    }
    s->moves[s->count] = move;
    s->undo[s->count] = undo;
    ++s->count;
    s->position = next;
    s->hashes[s->count] = next.hash;
    s->turnMs = now(s);
    if (result != AC_RESULT_NONE)
        end(s, result);
    publish(s);
    return AC_OK;
}
AcStatus ac_session_submit(AcSession *s, AcMoveRequest request) {
    if (!s)
        return AC_INVALID_ARGUMENT;
    uint64_t revision = s->revision;
    ac_session_tick(s);
    if (s->revision != revision)
        return AC_STALE_RESULT;
    if (s->phase != AC_SESSION_ACTIVE || ac_session_is_ai(&s->config, s->position.currentTurn))
        return AC_UNAVAILABLE;
    AcMove move;
    int resolved = ac_resolve_move_request(&s->position, request, &move);
    if (resolved)
        return resolved == AC_OUT_OF_MEMORY || resolved == AC_CAPACITY ? resolved : AC_ILLEGAL_MOVE;
    return commit_move(s, move);
}
AcStatus ac_session_submit_ai(AcSession *s, AcMove move, uint64_t revision, int budgetMs, int elapsedMs) {
    if (!s || budgetMs <= 0 || elapsedMs < 0)
        return AC_INVALID_ARGUMENT;
    ac_session_tick(s);
    if (revision != s->revision)
        return AC_STALE_RESULT;
    if (s->phase != AC_SESSION_ACTIVE || !ac_session_is_ai(&s->config, s->position.currentTurn))
        return AC_UNAVAILABLE;
    AcColor color = s->position.currentTurn;
    AcAIDifficulty d = color == AC_WHITE ? s->config.aiDifficultyWhite : s->config.aiDifficultyBlack;
    if (d == AC_DIFFICULTY_TOURNAMENT) {
        if (elapsedMs > s->tournament.remainingMs[color] || s->tournament.remainingMs[color] <= 0) {
            end(s, color == AC_WHITE ? AC_RESULT_BLACK_WIN : AC_RESULT_WHITE_WIN);
            publish(s);
            return AC_OK;
        }
        AcAITimeManager previous = s->tournament;
        ac_update_ai_tournament_time(&s->tournament, color, budgetMs, elapsedMs);
        AcStatus status = commit_move(s, move);
        if (status)
            s->tournament = previous;
        return status;
    }
    return commit_move(s, move);
}
AcStatus ac_session_undo(AcSession *s) {
    if (!s)
        return AC_INVALID_ARGUMENT;
    uint64_t revision = s->revision;
    ac_session_tick(s);
    if (revision != s->revision)
        return AC_STALE_RESULT;
    if (s->phase != AC_SESSION_ACTIVE || s->count == 0)
        return AC_UNAVAILABLE;
    int target = s->count - 1;
    if (s->config.mode == AC_MODE_HUMAN_VS_HUMAN)
        target = s->count >= 2 ? s->count - 2 : 0;
    else if (s->config.mode == AC_MODE_HUMAN_VS_COMPUTER) {
        while (target >= 0 && ((target % 2) == 0 ? AC_WHITE : AC_BLACK) != s->config.playerColor)
            --target;
        if (target < 0)
            return AC_UNAVAILABLE;
    }
    while (s->count > target) {
        --s->count;
        ac_position_unmake(&s->position, &s->undo[s->count]);
    }
    s->turnMs = now(s);
    s->hashes[s->count] = s->position.hash;
    publish(s);
    return AC_OK;
}
AcStatus ac_session_finish(AcSession *s) {
    if (!s)
        return AC_INVALID_ARGUMENT;
    if (s->phase == AC_SESSION_ACTIVE) {
        end(s, AC_RESULT_TERMINATED_BY_USER);
        publish(s);
    }
    return AC_OK;
}
AcStatus ac_session_promotion(const AcSession *s, AcMoveRequest r, int *needed) {
    if (!s || !needed)
        return AC_INVALID_ARGUMENT;
    *needed = 0;
    if (s->phase != AC_SESSION_ACTIVE)
        return AC_UNAVAILABLE;
    AcMove m;
    if (ac_resolve_move_request(&s->position, r, &m))
        return AC_ILLEGAL_MOVE;
    *needed = ac_is_promotion_special_move(m.specialType);
    return AC_OK;
}
int ac_session_ai_budget(const AcSession *s) {
    if (!s || s->phase != AC_SESSION_ACTIVE)
        return 0;
    AcColor c = s->position.currentTurn;
    AcAIDifficulty d = c == AC_WHITE ? s->config.aiDifficultyWhite : s->config.aiDifficultyBlack;
    if (d == AC_DIFFICULTY_TOURNAMENT) {
        AcAITimeManager copy = s->tournament;
        return ac_get_ai_tournament_budget_ms(&copy, c);
    }
    return ac_get_ai_time_budget_ms(&s->config, d);
}
const char *ac_status_message(AcStatus status) {
    switch (status) {
    case AC_OK:
        return "Ready";
    case AC_INVALID_ARGUMENT:
        return "Invalid input or configuration";
    case AC_ILLEGAL_MOVE:
        return "Illegal or ambiguous move";
    case AC_OUT_OF_MEMORY:
        return "Insufficient memory";
    case AC_CAPACITY:
        return "Move buffer capacity exceeded";
    case AC_CANCELLED:
        return "Search cancelled";
    case AC_STALE_RESULT:
        return "The position changed; please try again";
    case AC_UNAVAILABLE:
        return "Action unavailable";
    case AC_IO_ERROR:
        return "The game continues, but its log could not be written";
    default:
        return "Unknown error";
    }
}
