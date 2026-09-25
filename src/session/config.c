#include "anteater/rules.h"

#include <limits.h>
#include <stddef.h>

void ac_init_game_config_for_mode(AcGameConfig *config, AcGameMode mode) {
    if (config == NULL) {
        return;
    }

    config->mode = mode;
    config->timerEnabled = 0;
    config->aiTimeLimit = 0;
    config->initialTimeSeconds = 0;

    switch (mode) {
    case AC_MODE_HUMAN_VS_HUMAN:
        config->playerColor = AC_WHITE;
        config->aiDifficultyWhite = AC_DIFFICULTY_NONE;
        config->aiDifficultyBlack = AC_DIFFICULTY_NONE;
        break;
    case AC_MODE_HUMAN_VS_COMPUTER:
        config->playerColor = AC_WHITE;
        config->aiDifficultyWhite = AC_DIFFICULTY_NONE;
        config->aiDifficultyBlack = AC_DIFFICULTY_EASY;
        break;
    case AC_MODE_COMPUTER_VS_COMPUTER:
        config->playerColor = AC_EMPTY_COLOR;
        config->aiDifficultyWhite = AC_DIFFICULTY_EASY;
        config->aiDifficultyBlack = AC_DIFFICULTY_EASY;
        break;
    default:
        ac_init_game_config_for_mode(config, AC_MODE_HUMAN_VS_HUMAN);
        break;
    }
}

void ac_init_default_game_config(AcGameConfig *config) {
    ac_init_game_config_for_mode(config, AC_MODE_HUMAN_VS_HUMAN);
}

int ac_get_default_ai_time_budget_ms(AcAIDifficulty difficulty) {
    switch (difficulty) {
    case AC_DIFFICULTY_EASY:
        return 350;
    case AC_DIFFICULTY_MEDIUM:
        return 2200;
    case AC_DIFFICULTY_HARD:
    case AC_DIFFICULTY_EXPERIMENTAL:
        return 7000;
    case AC_DIFFICULTY_TOURNAMENT:
        return 14000;
    case AC_DIFFICULTY_NONE:
    default:
        return 0;
    }
}

int ac_get_ai_time_budget_ms(const AcGameConfig *config, AcAIDifficulty difficulty) {
    if (config != NULL && config->aiTimeLimit > 0) {
        if (config->aiTimeLimit > INT_MAX / 1000) {
            return INT_MAX;
        }
        return config->aiTimeLimit * 1000;
    }

    return ac_get_default_ai_time_budget_ms(difficulty);
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

int ac_get_required_ai_turn_timer_seconds(const AcGameConfig *config) {
    int maxBudgetMs = 0;

    if (config == NULL || config->mode == AC_MODE_HUMAN_VS_HUMAN) {
        return 0;
    }

    switch (config->mode) {
    case AC_MODE_HUMAN_VS_COMPUTER:
        if (config->playerColor == AC_WHITE) {
            maxBudgetMs = ac_get_ai_time_budget_ms(config, config->aiDifficultyBlack);
        } else {
            maxBudgetMs = ac_get_ai_time_budget_ms(config, config->aiDifficultyWhite);
        }
        break;
    case AC_MODE_COMPUTER_VS_COMPUTER:
        maxBudgetMs = max_int(ac_get_ai_time_budget_ms(config, config->aiDifficultyWhite),
                              ac_get_ai_time_budget_ms(config, config->aiDifficultyBlack));
        break;
    case AC_MODE_HUMAN_VS_HUMAN:
    default:
        break;
    }

    return required_seconds_for_budget_ms(maxBudgetMs);
}

int ac_is_ai_turn_timer_setting_valid(const AcGameConfig *config) {
    int requiredSeconds;

    if (config == NULL || !config->timerEnabled) {
        return 1;
    }

    requiredSeconds = ac_get_required_ai_turn_timer_seconds(config);
    return requiredSeconds <= 0 || config->initialTimeSeconds >= requiredSeconds;
}
