#include "session_adapter.h"
#include <algorithm>
#include <memory>

using namespace ac;
namespace ac {
static QString difficulty(Difficulty d) {
    switch (d) {
    case Difficulty::Easy:
        return "Easy";
    case Difficulty::Medium:
        return "Medium";
    case Difficulty::Hard:
        return "Hard";
    case Difficulty::Tournament:
        return "Tournament";
    default:
        return "Human";
    }
}
static QString coordinate(Square s) {
    return QString(QChar('A' + s.col)) + QString::number(8 - s.row);
}
static QString duration(int64_t seconds) {
    return QString("%1:%2:%3")
        .arg(seconds / 3600, 2, 10, QChar('0'))
        .arg(seconds / 60 % 60, 2, 10, QChar('0'))
        .arg(seconds % 60, 2, 10, QChar('0'));
}
SessionAdapter::SessionAdapter(const SessionOptions *options, QObject *parent, SessionLog log)
    : QObject(parent), log_(std::move(log)) {
    SessionOptions defaults{};
    defaults.clock = {monotonicMilliseconds, nullptr};
    auto created = Session::create(options ? *options : defaults);
    if (auto *owner = std::get_if<Session>(&created))
        session_.emplace(std::move(*owner));
    connect(&jobs_, &SearchJobs::completed, this, &SessionAdapter::searchCompleted);
    connect(&jobs_, &SearchJobs::busyChanged, this, &SessionAdapter::stateChanged);
    connect(&timer_, &QTimer::timeout, this, &SessionAdapter::tick);
    refresh();
    timer_.start(100);
}
SessionAdapter::~SessionAdapter() {
    timer_.stop();
    jobs_.shutdown();
}
SessionState SessionAdapter::state() const {
    return session_ ? session_->state() : SessionState{};
}
bool SessionAdapter::humanTurn() const {
    return page_ == Gameplay && state_.phase == SessionPhase::Active &&
           !session_is_ai(&state_.config, state_.position.currentTurn) && !closing_;
}
bool SessionAdapter::canUndo() const {
    return page_ == Gameplay && state_.historyCount > 0 && !closing_;
}
bool SessionAdapter::canHint() const {
    return humanTurn() && !busy() && !promotionPending_;
}
QString SessionAdapter::turnText() const {
    return QString(state_.position.currentTurn == Color::White ? "White" : "Black") + " to move";
}
QString SessionAdapter::clockText() const {
    return duration(state_.elapsedMs / 1000);
}
QString SessionAdapter::timerText(Color color) const {
    QString prefix = color == Color::White ? "White" : "Black";
    Difficulty d = color == Color::White ? state_.config.aiDifficultyWhite : state_.config.aiDifficultyBlack;
    if (d == Difficulty::Tournament)
        return prefix + " pool  " + duration((state_.tournamentRemainingMs[enum_index(color)] + 999) / 1000);
    if (!state_.config.timerEnabled)
        return prefix + "  —";
    return prefix + "  " + duration(state_.remaining[enum_index(color)]);
}
QString SessionAdapter::whiteTimer() const {
    return timerText(Color::White);
}
QString SessionAdapter::blackTimer() const {
    return timerText(Color::Black);
}
QString SessionAdapter::modeText() const {
    switch (state_.config.mode) {
    case GameMode::HumanVsComputer:
        return "Human vs AI";
    case GameMode::ComputerVsComputer:
        return "AI vs AI";
    default:
        return "Human vs Human";
    }
}
QString SessionAdapter::aiSummary() const {
    return "White: " + difficulty(state_.config.aiDifficultyWhite) +
           "  ·  Black: " + difficulty(state_.config.aiDifficultyBlack);
}
QString SessionAdapter::resultText() const {
    switch (state_.result) {
    case GameResult::WhiteWin:
        return "White wins";
    case GameResult::BlackWin:
        return "Black wins";
    case GameResult::Draw:
        return "Draw";
    case GameResult::TerminatedByUser:
        return "Game ended";
    default:
        return {};
    }
}
void SessionAdapter::report(Status s) {
    status_ = status_message(s);
    error_ = s != Status::Ok;
    emit stateChanged();
}
void SessionAdapter::invalidate() {
    ++generation_;
    jobs_.cancel();
    board_.hint(nullptr);
    failedCount_ = -1;
    if (promotionPending_) {
        promotionPending_ = false;
        emit promotionDismissed();
    }
}
void SessionAdapter::newGame() {
    if (closing_)
        return;
    invalidate();
    session_->finish();
    page_ = ModeMenu;
    report(Status::Ok);
    refresh();
}
void SessionAdapter::chooseMode(int mode) {
    if (closing_ || mode < value(GameMode::HumanVsHuman) || mode > value(GameMode::ComputerVsComputer))
        return;
    invalidate();
    page_ = Setup;
    report(Status::Ok);
    refresh();
}
void SessionAdapter::back() {
    if (closing_)
        return;
    invalidate();
    session_->finish();
    page_ = page_ == Setup ? ModeMenu : MainMenu;
    report(Status::Ok);
    refresh();
}
Status SessionAdapter::start(const GameConfig &c) {
    if (closing_ || !session_)
        return Status::Unavailable;
    Status s = session_->start(c);
    if (s == Status::Ok) {
        invalidate();
        page_ = Gameplay;
        from_.clear();
        to_.clear();
        updateHighlights();
    }
    report(s);
    refresh();
    return s;
}
bool SessionAdapter::startConfigured(int mode, int color, int white, int black, bool enabled, int seconds,
                                     int aiSeconds) {
    GameConfig c{};
    init_game_config_for_mode(&c, GameMode(mode));
    c.playerColor = Color(color);
    c.aiDifficultyWhite = Difficulty(white);
    c.aiDifficultyBlack = Difficulty(black);
    if (mode == value(GameMode::HumanVsHuman))
        c.aiDifficultyWhite = c.aiDifficultyBlack = Difficulty::None;
    if (mode == value(GameMode::HumanVsComputer)) {
        if (color == value(Color::White))
            c.aiDifficultyWhite = Difficulty::None;
        else
            c.aiDifficultyBlack = Difficulty::None;
    }
    c.timerEnabled = enabled;
    c.initialTimeSeconds = seconds;
    c.aiTimeLimit = aiSeconds;
    return start(c) == Status::Ok;
}
void SessionAdapter::setMoveFields(const QString &from, const QString &to) {
    if (closing_)
        return;
    from_ = from;
    to_ = to;
    board_.hint(nullptr);
    updateHighlights();
}
void SessionAdapter::updateHighlights() {
    std::array<bool, Rows * Columns> legal{};
    Square from = parse_position(from_.toUtf8().constData());
    fromValid_ = humanTurn() && validate_selection(&state_.position, from) == SelectionResult::Valid;
    try {
        if (fromValid_) {
            auto moves = std::make_unique<MoveList>();
            Status status = generate_legal_moves_for_position(&state_.position, from, moves.get());
            if (status == Status::Ok) {
                for (int i = 0; i < moves->count; ++i) {
                    Square to = moves->moves[i].to;
                    legal[size_t(to.row * Columns + to.col)] = true;
                }
            } else
                report(Status(status));
        }
    } catch (const std::bad_alloc &) {
        report(Status::OutOfMemory);
    }
    Square to = parse_position(to_.toUtf8().constData());
    toValid_ = is_valid_position(to) && legal[size_t(to.row * Columns + to.col)];
    board_.highlights(fromValid_ ? from : Square{-1, -1}, legal);
    emit moveFieldsChanged();
}
void SessionAdapter::selectSquare(int row, int column, int button) {
    Square square{row, column};
    if (!humanTurn() || !is_valid_position(square)) {
        report(Status::Unavailable);
        return;
    }
    // Retain the GTK left-selection/right-destination interaction.
    if (button == 1)
        setMoveFields(coordinate(square), {});
    else if (button == 2) {
        setMoveFields(from_, coordinate(square));
        if (toValid_)
            submitFields();
        else
            report(Status::IllegalMove);
    }
}
bool SessionAdapter::submitFields(int promotion) {
    if (!humanTurn()) {
        report(Status::Unavailable);
        return false;
    }
    MoveRequest request{};
    PromotionChoice choice = promotion ? PromotionChoice(promotion) : PromotionChoice::Queen;
    if (parse_move_request_fields(from_.toUtf8().constData(), to_.toUtf8().constData(), choice, &request) !=
        Status::Ok) {
        report(Status::InvalidArgument);
        return false;
    }
    auto needed = session_->promotion(request);
    if (auto *failure = std::get_if<Error>(&needed)) {
        report(failure->status);
        return false;
    }
    if (!promotion && std::get<bool>(needed)) {
        promotionPending_ = true;
        promotionRevision_ = state_.revision;
        emit promotionRequested();
        emit stateChanged();
        return false;
    }
    if (promotionPending_ && promotionRevision_ != state().revision) {
        cancelPromotion();
        report(Status::StaleResult);
        return false;
    }
    invalidate();
    Status s = session_->submit(request);
    if (s == Status::Ok) {
        from_.clear();
        to_.clear();
    }
    report(s);
    refresh();
    updateHighlights();
    return s == Status::Ok;
}
void SessionAdapter::cancelPromotion() {
    promotionPending_ = false;
    emit promotionDismissed();
    emit stateChanged();
}
void SessionAdapter::undo() {
    if (closing_)
        return;
    invalidate();
    report(session_->undo());
    refresh();
    updateHighlights();
}
bool SessionAdapter::hint() {
    if (!canHint()) {
        report(Status::Unavailable);
        return false;
    }
    auto owned = session_->snapshot();
    if (auto *failure = std::get_if<Error>(&owned)) {
        report(failure->status);
        return false;
    }
    const auto &s = std::get<SessionSnapshot>(owned);
    board_.hint(nullptr);
    bool started = false;
    try {
        started = jobs_.start(s, generation_, true, get_ai_time_budget_ms(&s.config, Difficulty::Medium), 8);
    } catch (const std::bad_alloc &) {
        report(Status::OutOfMemory);
    }
    if (started) {
        status_ = "Hint thinking…";
        error_ = false;
        emit stateChanged();
    }
    return started;
}
void SessionAdapter::finish() {
    if (closing_)
        return;
    invalidate();
    report(session_->finish());
    refresh();
}
bool SessionAdapter::requestClose() {
    if (!closing_) {
        closing_ = true;
        timer_.stop();
        invalidate();
        session_->finish();
        refresh();
        emit stateChanged();
    }
    return !busy();
}
void SessionAdapter::tick() {
    if (closing_ || !session_)
        return;
    SessionState previous = state();
    session_->tick();
    if (state().revision != previous.revision)
        invalidate();
    refresh();
}
void SessionAdapter::refresh() {
    if (!session_) {
        report(Status::OutOfMemory);
        return;
    }
    SessionState s = state();
    bool changed = s.revision != state_.revision;
    board_.update(s.position);
    state_ = s;
    if (page_ == Gameplay && s.phase == SessionPhase::Finished) {
        page_ = EndGame;
        invalidate();
    }
    if (changed) {
        jobs_.cancel();
        board_.hint(nullptr);
        if (promotionPending_ && promotionRevision_ != s.revision)
            cancelPromotion();
        updateHighlights();
    }
    const bool searchWanted = !closing_ && page_ == Gameplay && s.phase == SessionPhase::Active &&
                              session_is_ai(&s.config, s.position.currentTurn) && !busy() &&
                              !(failedCount_ == s.historyCount && failedTurn_ == s.position.currentTurn);
    // Clock polling takes no snapshot. Allocation or logging failures cannot
    // turn a command already accepted by the core into a failed command.
    if (changed || searchWanted) {
        auto owned = session_->snapshot();
        if (auto *failure = std::get_if<Error>(&owned)) {
            diagnostic_ = failure->status;
            report(diagnostic_);
            if (searchWanted) {
                failedCount_ = s.historyCount;
                failedTurn_ = s.position.currentTurn;
            }
        } else {
            try {
                const auto &snapshot = std::get<SessionSnapshot>(owned);
                if (changed) {
                    history_.update(snapshot);
                    diagnostic_ = log_.write(snapshot);
                    if (diagnostic_ != Status::Ok)
                        report(diagnostic_);
                }
                if (searchWanted) {
                    Difficulty d = s.position.currentTurn == Color::White ? s.config.aiDifficultyWhite
                                                                          : s.config.aiDifficultyBlack;
                    if (jobs_.start(snapshot, generation_, false, session_->ai_budget(), search_depth(d))) {
                        status_ = "AI thinking…";
                        error_ = false;
                    }
                }
            } catch (const std::bad_alloc &) {
                diagnostic_ = Status::OutOfMemory;
                report(diagnostic_);
                if (searchWanted) {
                    failedCount_ = s.historyCount;
                    failedTurn_ = s.position.currentTurn;
                }
            }
        }
    }
    emit stateChanged();
}
void SessionAdapter::searchCompleted() {
    const SearchOutcome out = jobs_.outcome();
    SessionState s = state();
    bool current =
        !closing_ && page_ == Gameplay && out.generation == generation_ && out.revision == s.revision && !out.cancelled;
    if (current && out.result.status == Status::Ok) {
        if (out.hint) {
            board_.hint(&out.result.move);
            status_ = "Hint: " + coordinate(out.result.move.from) + " → " + coordinate(out.result.move.to);
            error_ = false;
        } else
            report(session_->submit_ai(out.result.move, out.revision, out.budgetMs, out.result.elapsedMs));
    } else if (current && out.result.status != Status::Cancelled) {
        if (!out.hint) {
            failedCount_ = s.historyCount;
            failedTurn_ = s.position.currentTurn;
        }
        report(out.result.status);
    }
    if (closing_)
        emit closeReady();
    else
        refresh();
}
} // namespace ac
