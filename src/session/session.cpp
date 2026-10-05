#include "anteater/session.hpp"
#include "anteater/ai.hpp"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

namespace ac {

namespace detail {
struct SessionData {
    explicit SessionData(SessionOptions selected)
        : options(selected), moves(MaxMoves, selected.resource), undo(MaxMoves, selected.resource),
          hashes(MaxMoves + 1, selected.resource) {
        position_init(&position);
        init_default_game_config(&config);
        hashes[0] = position.hash;
        init_ai_time_manager(&tournament);
    }
    SessionOptions options;
    Position position{};
    GameConfig config{};
    SessionPhase phase = SessionPhase::Idle;
    GameResult result = GameResult::None;
    uint64_t revision = 0, gameId = 0;
    std::pmr::vector<Move> moves;
    std::pmr::vector<Undo> undo;
    std::pmr::vector<uint64_t> hashes;
    int count = 0;
    int64_t startMs = 0, turnMs = 0, finishedMs = 0;
    AITimeManager tournament{};
};
} // namespace detail
using detail::SessionData;
static int64_t now(const SessionData *s) noexcept {
    return s->options.clock.now(s->options.clock.context);
}
Session::Session(detail::OwnedObject<SessionData> data) noexcept : data_(std::move(data)) {
}
Session::~Session() = default;
Session::Session(Session &&) noexcept = default;
Session &Session::operator=(Session &&) noexcept = default;
Result<Session> Session::create(SessionOptions options) noexcept {
    if (!options.clock.now || !options.resource)
        return Error{Status::InvalidArgument, "A clock and memory resource are required"};
    try {
        return Session{detail::make_owned<SessionData>(options.resource, options)};
    } catch (const std::bad_alloc &) {
        return Error{Status::OutOfMemory, "Session allocation failed"};
    }
}
static int64_t nonnegative_delta(int64_t a, int64_t b) {
    return a > b ? a - b : 0;
}
int session_is_ai(const GameConfig *c, Color color) {
    return c && (c->mode == GameMode::ComputerVsComputer ||
                 (c->mode == GameMode::HumanVsComputer && color != c->playerColor));
}
SessionState Session::state() const noexcept {
    SessionState out{};
    const auto *s = data_.get();
    if (!s)
        return out;
    out.position = s->position;
    out.config = s->config;
    out.phase = s->phase;
    out.result = s->result;
    out.revision = s->revision;
    out.gameId = s->gameId;
    out.historyCount = s->count;
    const int64_t at = s->phase == SessionPhase::Finished ? s->finishedMs : now(s);
    out.elapsedMs = s->phase == SessionPhase::Idle ? 0 : nonnegative_delta(at, s->startMs);
    for (int c = 0; c < 2; ++c) {
        int64_t remaining = s->config.initialTimeSeconds;
        if (s->config.timerEnabled && c == value(s->position.currentTurn))
            remaining -= nonnegative_delta(at, s->turnMs) / 1000;
        out.remaining[c] = remaining < 0 ? 0 : remaining > INT_MAX ? INT_MAX : int(remaining);
        out.tournamentRemainingMs[c] = s->tournament.remainingMs[c];
    }
    return out;
}
Result<SessionSnapshot> Session::snapshot(std::pmr::memory_resource *resource) const noexcept {
    if (!data_)
        return Error{Status::Unavailable, "The Session has been moved"};
    if (!resource)
        resource = data_->options.resource;
    try {
        SessionSnapshot out{resource};
        static_cast<SessionState &>(out) = state();
        out.history.assign(data_->moves.begin(), data_->moves.begin() + data_->count);
        out.hashes.assign(data_->hashes.begin(), data_->hashes.begin() + data_->count + 1);
        return out;
    } catch (const std::bad_alloc &) {
        return Error{Status::OutOfMemory, "Snapshot allocation failed"};
    }
}
static void publish(SessionData *s) noexcept {
    ++s->revision;
}
static void end(SessionData *s, GameResult result) {
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
Status Session::start(const GameConfig &config) noexcept {
    auto *s = data_.get();
    const GameConfig *c = &config;
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
    s->startMs = s->turnMs = now(s);
    init_ai_time_manager(&s->tournament);
    publish(s);
    return Status::Ok;
}
Status Session::tick() noexcept {
    auto *s = data_.get();
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
static Status commit_move(SessionData *s, Move move) {
    if (s->count == MaxMoves) {
        end(s, GameResult::Draw);
        publish(s);
        return Status::Ok;
    }
    Position next = s->position;
    Undo undo;
    GameResult result;
    Status status = position_apply(&next, move, &undo, s->options.resource);
    if (status != Status::Ok)
        return status;
    status = position_result(&next, &result, s->options.resource);
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
Status Session::submit(MoveRequest request) noexcept {
    auto *s = data_.get();
    if (!s)
        return Status::InvalidArgument;
    uint64_t revision = s->revision;
    tick();
    if (s->revision != revision)
        return Status::StaleResult;
    if (s->phase != SessionPhase::Active || session_is_ai(&s->config, s->position.currentTurn))
        return Status::Unavailable;
    Move move;
    Status resolved = resolve_move_request(&s->position, request, &move, s->options.resource);
    if (resolved != Status::Ok)
        return resolved == Status::OutOfMemory || resolved == Status::Capacity ? static_cast<Status>(resolved)
                                                                               : Status::IllegalMove;
    return commit_move(s, move);
}
Status Session::submit_ai(Move move, uint64_t revision, int budgetMs, int elapsedMs) noexcept {
    auto *s = data_.get();
    if (!s || budgetMs <= 0 || elapsedMs < 0)
        return Status::InvalidArgument;
    tick();
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
Status Session::undo() noexcept {
    auto *s = data_.get();
    if (!s)
        return Status::InvalidArgument;
    uint64_t revision = s->revision;
    tick();
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
Status Session::finish() noexcept {
    auto *s = data_.get();
    if (!s)
        return Status::InvalidArgument;
    if (s->phase == SessionPhase::Active) {
        end(s, GameResult::TerminatedByUser);
        publish(s);
    }
    return Status::Ok;
}
Result<bool> Session::promotion(MoveRequest request) const noexcept {
    const auto *s = data_.get();
    if (!s || s->phase != SessionPhase::Active)
        return Error{Status::Unavailable, "Promotion is unavailable"};
    Move move{};
    Status status = resolve_move_request(&s->position, request, &move, s->options.resource);
    if (status != Status::Ok)
        return Error{status == Status::OutOfMemory || status == Status::Capacity ? status : Status::IllegalMove,
                     "Cannot resolve promotion"};
    return bool(is_promotion_special_move(move.specialType));
}
int Session::ai_budget() const noexcept {
    const auto *s = data_.get();
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
