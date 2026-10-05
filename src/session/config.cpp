#include "anteater/rules.hpp"

#include <limits.h>
#include <stddef.h>

namespace ac {

void init_game_config_for_mode(GameConfig *config, GameMode mode) {
    if (config == NULL) {
        return;
    }

    config->mode = mode;
    config->timerEnabled = 0;
    config->aiTimeLimit = 0;
    config->initialTimeSeconds = 0;

    switch (mode) {
    case GameMode::HumanVsHuman:
        config->playerColor = Color::White;
        config->aiDifficultyWhite = Difficulty::None;
        config->aiDifficultyBlack = Difficulty::None;
        break;
    case GameMode::HumanVsComputer:
        config->playerColor = Color::White;
        config->aiDifficultyWhite = Difficulty::None;
        config->aiDifficultyBlack = Difficulty::Easy;
        break;
    case GameMode::ComputerVsComputer:
        config->playerColor = Color::Empty;
        config->aiDifficultyWhite = Difficulty::Easy;
        config->aiDifficultyBlack = Difficulty::Easy;
        break;
    default:
        init_game_config_for_mode(config, GameMode::HumanVsHuman);
        break;
    }
}

void init_default_game_config(GameConfig *config) {
    init_game_config_for_mode(config, GameMode::HumanVsHuman);
}

int get_default_ai_time_budget_ms(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return 350;
    case Difficulty::Medium:
        return 2200;
    case Difficulty::Hard:
        return 7000;
    case Difficulty::Tournament:
        return 14000;
    case Difficulty::None:
    default:
        return 0;
    }
}

int get_ai_time_budget_ms(const GameConfig *config, Difficulty difficulty) {
    if (config != NULL && config->aiTimeLimit > 0) {
        if (config->aiTimeLimit > INT_MAX / 1000) {
            return INT_MAX;
        }
        return config->aiTimeLimit * 1000;
    }

    return get_default_ai_time_budget_ms(difficulty);
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

int get_required_ai_turn_timer_seconds(const GameConfig *config) {
    int maxBudgetMs = 0;

    if (config == NULL || config->mode == GameMode::HumanVsHuman) {
        return 0;
    }

    switch (config->mode) {
    case GameMode::HumanVsComputer:
        if (config->playerColor == Color::White) {
            maxBudgetMs = get_ai_time_budget_ms(config, config->aiDifficultyBlack);
        } else {
            maxBudgetMs = get_ai_time_budget_ms(config, config->aiDifficultyWhite);
        }
        break;
    case GameMode::ComputerVsComputer:
        maxBudgetMs = max_int(get_ai_time_budget_ms(config, config->aiDifficultyWhite),
                              get_ai_time_budget_ms(config, config->aiDifficultyBlack));
        break;
    case GameMode::HumanVsHuman:
    default:
        break;
    }

    return required_seconds_for_budget_ms(maxBudgetMs);
}

int is_ai_turn_timer_setting_valid(const GameConfig *config) {
    int requiredSeconds;

    if (config == NULL || !config->timerEnabled) {
        return 1;
    }

    requiredSeconds = get_required_ai_turn_timer_seconds(config);
    return requiredSeconds <= 0 || config->initialTimeSeconds >= requiredSeconds;
}

} // namespace ac
