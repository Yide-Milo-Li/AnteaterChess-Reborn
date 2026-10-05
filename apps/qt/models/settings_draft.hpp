#pragma once
#include "game_enums.hpp"
#include "anteater/policy.hpp"
namespace ac {
class SettingsDraft : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by ApplicationController")
    Q_PROPERTY(GameEnums::Mode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(GameEnums::Side playerColor READ playerColor WRITE setPlayerColor NOTIFY playerColorChanged)
    Q_PROPERTY(
        GameEnums::Level whiteDifficulty READ whiteDifficulty WRITE setWhiteDifficulty NOTIFY whiteDifficultyChanged)
    Q_PROPERTY(
        GameEnums::Level blackDifficulty READ blackDifficulty WRITE setBlackDifficulty NOTIFY blackDifficultyChanged)
    Q_PROPERTY(bool timerEnabled READ timerEnabled WRITE setTimerEnabled NOTIFY timerEnabledChanged)
    Q_PROPERTY(int turnSeconds READ turnSeconds WRITE setTurnSeconds NOTIFY turnSecondsChanged)
    Q_PROPERTY(int aiSeconds READ aiSeconds WRITE setAiSeconds NOTIFY aiSecondsChanged)
  public:
    GameEnums::Mode mode() const {
        return mode_;
    }
    void setMode(GameEnums::Mode value) {
        if (mode_ == value)
            return;
        mode_ = value;
        emit modeChanged();
    }
    GameEnums::Side playerColor() const {
        return playerColor_;
    }
    void setPlayerColor(GameEnums::Side value) {
        if (playerColor_ == value)
            return;
        playerColor_ = value;
        emit playerColorChanged();
    }
    GameEnums::Level whiteDifficulty() const {
        return whiteDifficulty_;
    }
    void setWhiteDifficulty(GameEnums::Level value) {
        if (whiteDifficulty_ == value)
            return;
        whiteDifficulty_ = value;
        emit whiteDifficultyChanged();
    }
    GameEnums::Level blackDifficulty() const {
        return blackDifficulty_;
    }
    void setBlackDifficulty(GameEnums::Level value) {
        if (blackDifficulty_ == value)
            return;
        blackDifficulty_ = value;
        emit blackDifficultyChanged();
    }
    bool timerEnabled() const {
        return timerEnabled_;
    }
    void setTimerEnabled(bool value) {
        if (timerEnabled_ == value)
            return;
        timerEnabled_ = value;
        emit timerEnabledChanged();
    }
    int turnSeconds() const {
        return turnSeconds_;
    }
    void setTurnSeconds(int value) {
        if (turnSeconds_ == value)
            return;
        turnSeconds_ = value;
        emit turnSecondsChanged();
    }
    int aiSeconds() const {
        return aiSeconds_;
    }
    void setAiSeconds(int value) {
        if (aiSeconds_ == value)
            return;
        aiSeconds_ = value;
        emit aiSecondsChanged();
    }
    void chooseMode(GameEnums::Mode mode) {
        setMode(mode);
        setPlayerColor(GameEnums::White);
        setWhiteDifficulty(GameEnums::Easy);
        setBlackDifficulty(GameEnums::Easy);
        setTimerEnabled(false);
        setTurnSeconds(0);
        setAiSeconds(0);
    }
    GameConfig config() const {
        GameConfig config{};
        config.mode = GameMode(mode_);
        config.playerColor = Color(playerColor_);
        config.aiDifficultyWhite = Difficulty(whiteDifficulty_);
        config.aiDifficultyBlack = Difficulty(blackDifficulty_);
        config.timerEnabled = timerEnabled_;
        config.initialTimeSeconds = turnSeconds_;
        config.aiTimeLimit = aiSeconds_;
        if (config.mode == GameMode::HumanVsHuman)
            config.aiDifficultyWhite = config.aiDifficultyBlack = Difficulty::None;
        else if (config.mode == GameMode::HumanVsComputer) {
            if (config.playerColor == Color::White)
                config.aiDifficultyWhite = Difficulty::None;
            else
                config.aiDifficultyBlack = Difficulty::None;
        }
        return config;
    }
    Status validate() const {
        if (!is_valid_difficulty(Difficulty(whiteDifficulty_)) || !is_valid_difficulty(Difficulty(blackDifficulty_)))
            return Status::InvalidArgument;
        return validate_config(config());
    }
  signals:
    void modeChanged();
    void playerColorChanged();
    void whiteDifficultyChanged();
    void blackDifficultyChanged();
    void timerEnabledChanged();
    void turnSecondsChanged();
    void aiSecondsChanged();

  private:
    GameEnums::Mode mode_ = GameEnums::HumanVsHuman;
    GameEnums::Side playerColor_ = GameEnums::White;
    GameEnums::Level whiteDifficulty_ = GameEnums::Easy;
    GameEnums::Level blackDifficulty_ = GameEnums::Easy;
    bool timerEnabled_ = false;
    int turnSeconds_ = 0;
    int aiSeconds_ = 0;
};
} // namespace ac
