#include "core/gameconfig.h"

#include <limits.h>
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

int getDefaultAITimeBudgetMs(AIDifficulty difficulty) {
    switch (difficulty) {
        case DIFFICULTY_EASY:
            return 350;
        case DIFFICULTY_MEDIUM:
            return 2200;
        case DIFFICULTY_HARD:
        case DIFFICULTY_EXPERIMENTAL:
            return 7000;
        case DIFFICULTY_TOURNAMENT:
            return 14000;
        case DIFFICULTY_NONE:
        default:
            return 0;
    }
}

int getAITimeBudgetMs(const GameConfig *config, AIDifficulty difficulty) {
    if (config != NULL && config->aiTimeLimit > 0) {
        if (config->aiTimeLimit > INT_MAX / 1000) {
            return INT_MAX;
        }
        return config->aiTimeLimit * 1000;
    }

    return getDefaultAITimeBudgetMs(difficulty);
}

static int max_int(int left, int right) {
    return (left > right) ? left : right;
}

static int required_seconds_for_budget_ms(int budgetMs) {
    int paddedBudgetMs;

    if (budgetMs <= 0) {
        return 0;
    }

    if (budgetMs > INT_MAX - 1499) {
        return INT_MAX / 1000;
    }

    paddedBudgetMs = budgetMs + 500;
    return (paddedBudgetMs + 999) / 1000;
}

int getRequiredAITurnTimerSeconds(const GameConfig *config) {
    int maxBudgetMs = 0;

    if (config == NULL || config->mode == MODE_HUMAN_VS_HUMAN) {
        return 0;
    }

    switch (config->mode) {
        case MODE_HUMAN_VS_COMPUTER:
            if (config->playerColor == WHITE) {
                maxBudgetMs = getAITimeBudgetMs(config, config->aiDifficultyBlack);
            } else {
                maxBudgetMs = getAITimeBudgetMs(config, config->aiDifficultyWhite);
            }
            break;
        case MODE_COMPUTER_VS_COMPUTER:
            maxBudgetMs = max_int(
                getAITimeBudgetMs(config, config->aiDifficultyWhite),
                getAITimeBudgetMs(config, config->aiDifficultyBlack));
            break;
        case MODE_HUMAN_VS_HUMAN:
        default:
            break;
    }

    return required_seconds_for_budget_ms(maxBudgetMs);
}

int isAITurnTimerSettingValid(const GameConfig *config) {
    int requiredSeconds;

    if (config == NULL || !config->timerEnabled) {
        return 1;
    }

    requiredSeconds = getRequiredAITurnTimerSeconds(config);
    return requiredSeconds <= 0 || config->initialTimeSeconds >= requiredSeconds;
}
