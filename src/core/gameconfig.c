#include "core/gameconfig.h"

#include <stddef.h>

void initGameConfigForMode(GameConfig *config, GameMode mode) {
    if (config == NULL) {
        return;
    }

    config->mode = mode;
    config->timerEnabled = 0;
    config->aiTimeLimit = 0;
    config->initialTimeSeconds = 0;

    switch (mode) {
        case MODE_HUMAN_VS_HUMAN:
            config->playerColor = WHITE;
            config->aiDifficultyWhite = DIFFICULTY_NONE;
            config->aiDifficultyBlack = DIFFICULTY_NONE;
            break;
        case MODE_HUMAN_VS_COMPUTER:
            config->playerColor = WHITE;
            config->aiDifficultyWhite = DIFFICULTY_NONE;
            config->aiDifficultyBlack = DIFFICULTY_EASY;
            break;
        case MODE_COMPUTER_VS_COMPUTER:
            config->playerColor = EMPTY_COLOR;
            config->aiDifficultyWhite = DIFFICULTY_EASY;
            config->aiDifficultyBlack = DIFFICULTY_EASY;
            break;
        default:
            initGameConfigForMode(config, MODE_HUMAN_VS_HUMAN);
            break;
    }
}

void initDefaultGameConfig(GameConfig *config) {
    initGameConfigForMode(config, MODE_HUMAN_VS_HUMAN);
}
