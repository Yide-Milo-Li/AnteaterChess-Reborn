#include "ui/game_setup_menu.h"
#include "core/piece.h"

static GameConfig setup_config;

void initGameSetupConfig(GameMode mode) {
    setup_config.mode = mode;
    if (mode == MODE_COMPUTER_VS_COMPUTER) {
        setup_config.playerColor = EMPTY_COLOR;
    } else {
        setup_config.playerColor = WHITE; // default
    }
    if (mode == MODE_HUMAN_VS_HUMAN) {
        setup_config.aiDifficultyWhite = DIFFICULTY_NONE;
        setup_config.aiDifficultyBlack = DIFFICULTY_NONE;
    } else if (mode == MODE_HUMAN_VS_COMPUTER) {
        setup_config.aiDifficultyWhite = DIFFICULTY_NONE; // human white
        setup_config.aiDifficultyBlack = DIFFICULTY_EASY; // AI black
    } else { // MODE_COMPUTER_VS_COMPUTER
        setup_config.aiDifficultyWhite = DIFFICULTY_EASY;
        setup_config.aiDifficultyBlack = DIFFICULTY_EASY;
    }
    setup_config.timerEnabled = 0;
    setup_config.initialTimeSeconds = 0;
    setup_config.aiTimeLimit = 0;
}

void setPlayerColor(Color color) {
    setup_config.playerColor = color;
}

void setAIDifficultyWhite(AIDifficulty diff) {
    setup_config.aiDifficultyWhite = diff;
}

void setAIDifficultyBlack(AIDifficulty diff) {
    setup_config.aiDifficultyBlack = diff;
}

void setTimerEnabled(int enabled) {
    setup_config.timerEnabled = enabled;
}

void setInitialTimeSeconds(int seconds) {
    setup_config.initialTimeSeconds = seconds;
}

int getGameSetupConfig(GameConfig *config) {
    if (!config) return -1;
    *config = setup_config;
    return 0;
}
