#include "status_model.hpp"
namespace ac {
static QString difficulty(Difficulty value) {
    switch (value) {
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
void StatusModel::update(const SessionState &state, bool gameplay, bool busy, bool closing, bool promotion) {
    bool human = gameplay && state.phase == SessionPhase::Active &&
                 !is_ai_turn(&state.config, state.position.currentTurn) && !closing;
    bool undo = gameplay && state.historyCount > 0 && !closing, hint = human && !busy && !promotion;
    if (humanTurn_ != human || canUndo_ != undo || canHint_ != hint) {
        humanTurn_ = human;
        canUndo_ = undo;
        canHint_ = hint;
        emit availabilityChanged();
    }
    if (count_ != state.historyCount) {
        count_ = state.historyCount;
        emit historyCountChanged();
    }
    QString turn = QString(state.position.currentTurn == Color::White ? "White" : "Black") + " to move";
    if (turn_ != turn) {
        turn_ = turn;
        emit turnChanged();
    }
    QString mode = state.config.mode == GameMode::HumanVsComputer      ? "Human vs AI"
                   : state.config.mode == GameMode::ComputerVsComputer ? "AI vs AI"
                                                                       : "Human vs Human";
    QString ai = "White: " + difficulty(state.config.aiDifficultyWhite) +
                 "  ·  Black: " + difficulty(state.config.aiDifficultyBlack);
    if (mode_ != mode || ai_ != ai) {
        mode_ = mode;
        ai_ = ai;
        emit configurationChanged();
    }
    QString result;
    switch (state.result) {
    case GameResult::WhiteWin:
        result = "White wins";
        break;
    case GameResult::BlackWin:
        result = "Black wins";
        break;
    case GameResult::Draw:
        result = "Draw";
        break;
    case GameResult::TerminatedByUser:
        result = "Game ended";
        break;
    default:
        break;
    }
    if (result_ != result) {
        result_ = result;
        emit resultChanged();
    }
}
} // namespace ac
