#include "clock_model.hpp"
namespace ac {
static QString duration(int64_t seconds) {
    return QString("%1:%2:%3")
        .arg(seconds / 3600, 2, 10, QChar('0'))
        .arg(seconds / 60 % 60, 2, 10, QChar('0'))
        .arg(seconds % 60, 2, 10, QChar('0'));
}
static QString timer(const SessionState &state, Color color) {
    QString prefix = color == Color::White ? "White" : "Black";
    Difficulty difficulty = color == Color::White ? state.config.aiDifficultyWhite : state.config.aiDifficultyBlack;
    if (difficulty == Difficulty::Tournament)
        return prefix + " pool  " + duration((state.tournamentRemainingMs[enum_index(color)] + 999) / 1000);
    if (!state.config.timerEnabled)
        return prefix + "  —";
    return prefix + "  " + duration(state.remaining[enum_index(color)]);
}
void ClockModel::update(const SessionState &state) {
    auto elapsed = duration(state.elapsedMs / 1000), white = timer(state, Color::White),
         black = timer(state, Color::Black);
    if (elapsed_ != elapsed) {
        elapsed_ = elapsed;
        emit elapsedChanged();
    }
    if (white_ != white) {
        white_ = white;
        emit whiteChanged();
    }
    if (black_ != black) {
        black_ = black;
        emit blackChanged();
    }
}
} // namespace ac
