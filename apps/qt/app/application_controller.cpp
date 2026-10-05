#include "application_controller.hpp"
#include <algorithm>
#include <memory>

using namespace ac;
namespace ac {
static QString coordinate(Square s) {
    return QString(QChar('A' + s.col)) + QString::number(8 - s.row);
}
ApplicationController::ApplicationController(QObject *parent) : ApplicationController(nullptr, parent) {
}
ApplicationController::ApplicationController(const SessionOptions *options, QObject *parent, SessionLog log,
                                             std::pmr::memory_resource *searchResource)
    : QObject(parent), log_(std::move(log)), jobs_(nullptr, searchResource) {
    SessionOptions defaults{};
    defaults.clock = {monotonicMilliseconds, nullptr};
    auto created = Session::create(options ? *options : defaults);
    if (auto *owner = std::get_if<Session>(&created))
        session_.emplace(std::move(*owner));
    connect(&jobs_, &SearchJobs::completed, this, &ApplicationController::searchCompleted);
    connect(&jobs_, &SearchJobs::busyChanged, this, [this] {
        emit busyChanged();
        publishStatus();
    });
    connect(&timer_, &QTimer::timeout, this, &ApplicationController::tick);
    refresh();
    timer_.start(100);
}
ApplicationController::~ApplicationController() {
    closing_ = true;
    timer_.stop();
    jobs_.shutdown();
}
SessionState ApplicationController::state() const {
    return session_ ? session_->state() : SessionState{};
}
void ApplicationController::report(Status status) {
    game_.setMessage(status_message(status), status != Status::Ok);
}
void ApplicationController::publishStatus() {
    game_.update(state_, page_ == Gameplay, busy(), closing_, promotionPending_);
}
void ApplicationController::setPage(Page page) {
    if (page_ == page)
        return;
    page_ = page;
    emit pageChanged();
    publishStatus();
}
void ApplicationController::invalidate() {
    ++generation_;
    jobs_.cancel();
    board_.hint(nullptr);
    failedCount_ = -1;
    if (promotionPending_) {
        promotionPending_ = false;
        emit promotionDismissed();
    }
}
void ApplicationController::newGame() {
    if (closing_)
        return;
    invalidate();
    session_->finish();
    setPage(ModeMenu);
    report(Status::Ok);
    refresh();
}
void ApplicationController::chooseMode(GameEnums::Mode mode) {
    if (closing_ || mode < value(GameMode::HumanVsHuman) || mode > value(GameMode::ComputerVsComputer))
        return;
    invalidate();
    settings_.chooseMode(mode);
    setPage(Setup);
    report(Status::Ok);
    refresh();
}
void ApplicationController::back() {
    if (closing_)
        return;
    invalidate();
    session_->finish();
    setPage(page_ == Setup ? ModeMenu : MainMenu);
    report(Status::Ok);
    refresh();
}
Status ApplicationController::start(const GameConfig &c) {
    if (closing_ || !session_)
        return Status::Unavailable;
    Status s = session_->start(c);
    if (s == Status::Ok) {
        invalidate();
        setPage(Gameplay);
        input_.setFields({}, {});
        updateHighlights();
    }
    report(s);
    refresh();
    return s;
}
bool ApplicationController::startDraft() {
    auto status = settings_.validate();
    if (status != Status::Ok) {
        report(status);
        return false;
    }
    return start(settings_.config()) == Status::Ok;
}
void ApplicationController::setMoveFields(const QString &from, const QString &to) {
    if (closing_)
        return;
    input_.setFields(from, to);
    board_.hint(nullptr);
    updateHighlights();
}
void ApplicationController::updateHighlights() {
    std::array<bool, Rows * Columns> legal{};
    Square from = parse_position(input_.fromText().toUtf8().constData());
    bool fromValid = humanTurn() && validate_selection(&state_.position, from) == SelectionResult::Valid;
    try {
        if (fromValid) {
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
    Square to = parse_position(input_.toText().toUtf8().constData());
    bool toValid = is_valid_position(to) && legal[size_t(to.row * Columns + to.col)];
    board_.highlights(fromValid ? from : Square{-1, -1}, legal);
    input_.setValidity(fromValid, toValid);
}
void ApplicationController::selectSquare(int row, int column, Qt::MouseButton button) {
    Square square{row, column};
    if (!humanTurn() || !is_valid_position(square)) {
        report(Status::Unavailable);
        return;
    }
    // Retain the GTK left-selection/right-destination interaction.
    if (button == Qt::LeftButton)
        setMoveFields(coordinate(square), {});
    else if (button == Qt::RightButton) {
        setMoveFields(input_.fromText(), coordinate(square));
        if (input_.toValid())
            submitFields();
        else
            report(Status::IllegalMove);
    }
}
bool ApplicationController::submitFields(GameEnums::Promotion promotion) {
    if (!humanTurn()) {
        report(Status::Unavailable);
        return false;
    }
    MoveRequest request{};
    PromotionChoice choice = promotion ? PromotionChoice(promotion) : PromotionChoice::Queen;
    if (parse_move_request_fields(input_.fromText().toUtf8().constData(), input_.toText().toUtf8().constData(), choice,
                                  &request) != Status::Ok) {
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
        publishStatus();
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
        input_.setFields({}, {});
    }
    report(s);
    refresh();
    updateHighlights();
    return s == Status::Ok;
}
void ApplicationController::cancelPromotion() {
    promotionPending_ = false;
    emit promotionDismissed();
    publishStatus();
}
void ApplicationController::undo() {
    if (closing_)
        return;
    invalidate();
    report(session_->undo());
    refresh();
    updateHighlights();
}
bool ApplicationController::hint() {
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
    const auto status = jobs_.start(s, generation_, true, get_ai_time_budget_ms(&s.config, Difficulty::Medium), 8);
    const bool started = status == Status::Ok;
    if (!started)
        report(status);
    if (started) {
        game_.setMessage("Hint thinking…", false);
        publishStatus();
    }
    return started;
}
void ApplicationController::finish() {
    if (closing_)
        return;
    invalidate();
    report(session_->finish());
    refresh();
}
bool ApplicationController::requestClose() {
    if (!closing_) {
        closing_ = true;
        timer_.stop();
        invalidate();
        session_->finish();
        refresh();
        publishStatus();
    }
    return !busy();
}
void ApplicationController::tick() {
    if (closing_ || !session_)
        return;
    const auto before = state_.revision;
    session_->tick();
    auto current = state();
    if (current.revision != before) {
        invalidate();
        refresh();
    } else {
        state_ = current;
        clocks_.update(current);
    }
}
void ApplicationController::refresh() {
    if (!session_) {
        report(Status::OutOfMemory);
        return;
    }
    SessionState s = state();
    bool changed = s.revision != state_.revision;
    if (changed || state_.position.hash != s.position.hash)
        board_.update(s.position);
    state_ = s;
    clocks_.update(s);
    publishStatus();
    if (page_ == Gameplay && s.phase == SessionPhase::Finished) {
        setPage(EndGame);
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
                              is_ai_turn(&s.config, s.position.currentTurn) && !busy() &&
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
                    const auto started =
                        jobs_.start(snapshot, generation_, false, session_->ai_budget(), search_depth_limit(d));
                    if (started == Status::Ok) {
                        game_.setMessage("AI thinking…", false);
                    } else {
                        failedCount_ = s.historyCount;
                        failedTurn_ = s.position.currentTurn;
                        report(started);
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
    publishStatus();
}
void ApplicationController::searchCompleted() {
    const SearchOutcome out = jobs_.outcome();
    SessionState s = state();
    bool current = !closing_ && page_ == Gameplay && out.gameId == s.gameId && out.generation == generation_ &&
                   out.revision == s.revision && !out.cancelled;
    if (current && out.result.status == Status::Ok) {
        if (out.hint) {
            board_.hint(&out.result.move);
            game_.setMessage("Hint: " + coordinate(out.result.move.from) + " → " + coordinate(out.result.move.to),
                             false);
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
