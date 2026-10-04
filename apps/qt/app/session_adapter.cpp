#include "session_adapter.h"
#include <algorithm>
#include <memory>
namespace ac {
static QString difficulty(AcAIDifficulty d) {
    switch (d) {
    case AC_DIFFICULTY_EASY: return "Easy";
    case AC_DIFFICULTY_MEDIUM: return "Medium";
    case AC_DIFFICULTY_HARD: return "Hard";
    case AC_DIFFICULTY_TOURNAMENT: return "Tournament";
    default: return "Human";
    }
}
static QString coordinate(AcSquare s) { return QString(QChar('A'+s.col)) + QString::number(8-s.row); }
static QString duration(int64_t seconds) {
    return QString("%1:%2:%3").arg(seconds/3600,2,10,QChar('0'))
        .arg(seconds/60%60,2,10,QChar('0')).arg(seconds%60,2,10,QChar('0'));
}
SessionAdapter::SessionAdapter(const AcSessionOptions *options, QObject *parent)
    : QObject(parent), log_(SessionLog::besideExecutable()) {
    AcSessionOptions defaults{};
    defaults.clock = {monotonicMilliseconds,nullptr};
    defaults.log = SessionLog::writeCallback; defaults.logContext = &log_;
    session_ = ac_session_create(options ? options : &defaults);
    connect(&jobs_, &SearchJobs::completed, this, &SessionAdapter::searchCompleted);
    connect(&jobs_, &SearchJobs::busyChanged, this, &SessionAdapter::stateChanged);
    connect(&timer_, &QTimer::timeout, this, &SessionAdapter::tick);
    refresh();
    timer_.start(100);
}
SessionAdapter::~SessionAdapter() {
    timer_.stop();
    jobs_.shutdown();
    ac_session_destroy(session_);
}
AcSnapshot SessionAdapter::snapshot() const {
    AcSnapshot s{};
    if (session_) ac_session_snapshot(session_, &s);
    return s;
}
bool SessionAdapter::humanTurn() const {
    return page_ == Gameplay && state_.phase == AC_SESSION_ACTIVE &&
        !ac_session_is_ai(&state_.config,state_.position.currentTurn) && !closing_;
}
bool SessionAdapter::canUndo() const { return page_ == Gameplay && state_.historyCount > 0 && !closing_; }
bool SessionAdapter::canHint() const { return humanTurn() && !busy() && !promotionPending_; }
QString SessionAdapter::turnText() const {
    return QString(state_.position.currentTurn == AC_WHITE ? "White" : "Black") + " to move";
}
QString SessionAdapter::clockText() const { return duration(state_.elapsedMs/1000); }
QString SessionAdapter::timerText(AcColor color) const {
    QString prefix = color == AC_WHITE ? "White" : "Black";
    AcAIDifficulty d = color == AC_WHITE ? state_.config.aiDifficultyWhite : state_.config.aiDifficultyBlack;
    if (d == AC_DIFFICULTY_TOURNAMENT)
        return prefix + " pool  " + duration((state_.tournamentRemainingMs[color]+999)/1000);
    if (!state_.config.timerEnabled) return prefix + "  —";
    return prefix + "  " + duration(state_.remaining[color]);
}
QString SessionAdapter::whiteTimer() const { return timerText(AC_WHITE); }
QString SessionAdapter::blackTimer() const { return timerText(AC_BLACK); }
QString SessionAdapter::modeText() const {
    switch (state_.config.mode) {
    case AC_MODE_HUMAN_VS_COMPUTER: return "Human vs AI";
    case AC_MODE_COMPUTER_VS_COMPUTER: return "AI vs AI";
    default: return "Human vs Human";
    }
}
QString SessionAdapter::aiSummary() const {
    return "White: " + difficulty(state_.config.aiDifficultyWhite) + "  ·  Black: " + difficulty(state_.config.aiDifficultyBlack);
}
QString SessionAdapter::resultText() const {
    switch (state_.result) {
    case AC_RESULT_WHITE_WIN: return "White wins";
    case AC_RESULT_BLACK_WIN: return "Black wins";
    case AC_RESULT_DRAW: return "Draw";
    case AC_RESULT_TERMINATED_BY_USER: return "Game ended";
    default: return {};
    }
}
void SessionAdapter::report(AcStatus s) {
    status_ = ac_status_message(s); error_ = s != AC_OK;
    emit stateChanged();
}
void SessionAdapter::invalidate() {
    ++generation_; jobs_.cancel(); board_.hint(nullptr);
    failedCount_ = -1;
    if (promotionPending_) { promotionPending_ = false; emit promotionDismissed(); }
}
void SessionAdapter::newGame() {
    if (closing_) return;
    invalidate(); ac_session_finish(session_); page_ = ModeMenu; report(AC_OK); refresh();
}
void SessionAdapter::chooseMode(int mode) {
    if (closing_ || mode < AC_MODE_HUMAN_VS_HUMAN || mode > AC_MODE_COMPUTER_VS_COMPUTER) return;
    invalidate(); page_ = Setup; report(AC_OK); refresh();
}
void SessionAdapter::back() {
    if (closing_) return;
    invalidate(); ac_session_finish(session_); page_ = page_ == Setup ? ModeMenu : MainMenu;
    report(AC_OK); refresh();
}
AcStatus SessionAdapter::start(const AcGameConfig &c) {
    if (closing_) return AC_UNAVAILABLE;
    AcStatus s = ac_session_start(session_, &c);
    if (s == AC_OK) {
        invalidate(); page_ = Gameplay; from_.clear(); to_.clear();
        updateHighlights();
    }
    report(s); refresh(); return s;
}
bool SessionAdapter::startConfigured(int mode, int color, int white, int black, bool enabled, int seconds, int aiSeconds) {
    AcGameConfig c{};
    ac_init_game_config_for_mode(&c,AcGameMode(mode));
    c.playerColor = AcColor(color);
    c.aiDifficultyWhite = AcAIDifficulty(white); c.aiDifficultyBlack = AcAIDifficulty(black);
    if (mode == AC_MODE_HUMAN_VS_HUMAN) c.aiDifficultyWhite = c.aiDifficultyBlack = AC_DIFFICULTY_NONE;
    if (mode == AC_MODE_HUMAN_VS_COMPUTER) {
        if (color == AC_WHITE) c.aiDifficultyWhite = AC_DIFFICULTY_NONE;
        else c.aiDifficultyBlack = AC_DIFFICULTY_NONE;
    }
    c.timerEnabled = enabled; c.initialTimeSeconds = seconds; c.aiTimeLimit = aiSeconds;
    return start(c) == AC_OK;
}
void SessionAdapter::setMoveFields(const QString &from, const QString &to) {
    if (closing_) return;
    from_ = from; to_ = to; board_.hint(nullptr); updateHighlights();
}
void SessionAdapter::updateHighlights() {
    std::array<bool,AC_ROWS*AC_COLS> legal{};
    AcSquare from = ac_parse_position(from_.toUtf8().constData());
    fromValid_ = humanTurn() && ac_validate_selection(&state_.position,from) == AC_SELECT_VALID;
    if (fromValid_) {
        auto moves = std::make_unique<AcMoveList>();
        int status = ac_generate_legal_moves_for_position(&state_.position,from,moves.get());
        if (status == AC_OK) {
            for (int i = 0; i < moves->count; ++i) {
                AcSquare to = moves->moves[i].to;
                legal[size_t(to.row*AC_COLS+to.col)] = true;
            }
        } else report(AcStatus(status));
    }
    AcSquare to = ac_parse_position(to_.toUtf8().constData());
    toValid_ = ac_is_valid_position(to) && legal[size_t(to.row*AC_COLS+to.col)];
    board_.highlights(fromValid_ ? from : AcSquare{-1,-1},legal);
    emit moveFieldsChanged();
}
void SessionAdapter::selectSquare(int row, int column, int button) {
    AcSquare square{row,column};
    if (!humanTurn() || !ac_is_valid_position(square)) { report(AC_UNAVAILABLE); return; }
    // Retain the GTK left-selection/right-destination interaction.
    if (button == 1) setMoveFields(coordinate(square),{});
    else if (button == 2) { setMoveFields(from_,coordinate(square)); if (toValid_) submitFields(); else report(AC_ILLEGAL_MOVE); }
}
bool SessionAdapter::submitFields(int promotion) {
    if (!humanTurn()) { report(AC_UNAVAILABLE); return false; }
    AcMoveRequest request{};
    AcPromotionChoice choice = promotion ? AcPromotionChoice(promotion) : AC_PROMOTION_CHOICE_QUEEN;
    if (ac_parse_move_request_fields(from_.toUtf8().constData(),to_.toUtf8().constData(),choice,&request)) {
        report(AC_INVALID_ARGUMENT); return false;
    }
    int needed = 0;
    if (!promotion && ac_session_promotion(session_,request,&needed) == AC_OK && needed) {
        promotionPending_ = true; promotionRevision_ = state_.revision;
        emit promotionRequested(); emit stateChanged(); return false;
    }
    if (promotionPending_ && promotionRevision_ != snapshot().revision) {
        cancelPromotion(); report(AC_STALE_RESULT); return false;
    }
    invalidate();
    AcStatus s = ac_session_submit(session_,request);
    if (s == AC_OK) { from_.clear(); to_.clear(); }
    report(s); refresh(); updateHighlights(); return s == AC_OK;
}
void SessionAdapter::cancelPromotion() {
    promotionPending_ = false; emit promotionDismissed(); emit stateChanged();
}
void SessionAdapter::undo() {
    if (closing_) return;
    invalidate(); report(ac_session_undo(session_)); refresh(); updateHighlights();
}
bool SessionAdapter::hint() {
    if (!canHint()) { report(AC_UNAVAILABLE); return false; }
    AcSnapshot s = snapshot(); board_.hint(nullptr);
    bool started = jobs_.start(s,generation_,true,ac_get_ai_time_budget_ms(&s.config,AC_DIFFICULTY_MEDIUM),8);
    if (started) { status_ = "Hint thinking…"; error_ = false; emit stateChanged(); }
    return started;
}
void SessionAdapter::finish() {
    if (closing_) return;
    invalidate(); report(ac_session_finish(session_)); refresh();
}
bool SessionAdapter::requestClose() {
    if (!closing_) {
        closing_ = true; timer_.stop(); invalidate(); ac_session_finish(session_);
        emit stateChanged();
    }
    return !busy();
}
void SessionAdapter::tick() {
    if (closing_ || !session_) return;
    AcSnapshot previous = snapshot();
    ac_session_tick(session_);
    if (snapshot().revision != previous.revision) invalidate();
    refresh();
}
void SessionAdapter::refresh() {
    if (!session_) { report(AC_OUT_OF_MEMORY); return; }
    AcSnapshot s = snapshot();
    bool changed = s.revision != state_.revision;
    board_.update(s.position); history_.update(s);
    state_ = s; state_.history = nullptr; state_.hashes = nullptr;
    if (page_ == Gameplay && s.phase == AC_SESSION_FINISHED) { page_ = EndGame; invalidate(); }
    if (changed) {
        jobs_.cancel(); board_.hint(nullptr);
        if (promotionPending_ && promotionRevision_ != s.revision) cancelPromotion();
        updateHighlights();
    }
    if (s.diagnostic != AC_OK) { status_ = ac_status_message(s.diagnostic); error_ = true; }
    if (!closing_ && page_ == Gameplay && ac_session_is_ai(&s.config,s.position.currentTurn) && !busy() &&
        !(failedCount_ == s.historyCount && failedTurn_ == s.position.currentTurn)) {
        AcAIDifficulty d = s.position.currentTurn == AC_WHITE ? s.config.aiDifficultyWhite : s.config.aiDifficultyBlack;
        if (jobs_.start(s,generation_,false,ac_session_ai_budget(session_),ac_search_depth(d))) {
            status_ = "AI thinking…"; error_ = false;
        }
    }
    emit stateChanged();
}
void SessionAdapter::searchCompleted() {
    const SearchOutcome out = jobs_.outcome();
    AcSnapshot s = snapshot();
    bool current = !closing_ && page_ == Gameplay && out.generation == generation_ &&
        out.revision == s.revision && !out.cancelled;
    if (current && out.result.status == AC_OK) {
        if (out.hint) {
            board_.hint(&out.result.move);
            status_ = "Hint: " + coordinate(out.result.move.from) + " → " + coordinate(out.result.move.to);
            error_ = false;
        } else report(ac_session_submit_ai(session_,out.result.move,out.revision,out.budgetMs,out.result.elapsedMs));
    } else if (current && out.result.status != AC_CANCELLED) {
        if (!out.hint) { failedCount_ = s.historyCount; failedTurn_ = s.position.currentTurn; }
        report(out.result.status);
    }
    if (closing_) emit closeReady();
    else refresh();
}
}
