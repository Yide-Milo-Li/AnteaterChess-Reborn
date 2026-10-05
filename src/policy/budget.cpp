#include "anteater/policy.hpp"

namespace ac {
#define AI_TOURNAMENT_TOTAL_MS 600999
#define AI_TOURNAMENT_RESERVE_MS 30000
#define AI_TOURNAMENT_BASE_MS 7000
#define AI_TOURNAMENT_MAX_MS 10000
#define AI_TOURNAMENT_MAX_EXTRA_MS 3000
#define AI_TOURNAMENT_POOL_CAP_MS 180000
#define AI_MIN_MOVE_BUDGET_MS 300

static int ai_clamp_int(int value, int minValue, int maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

static int ai_color_time_index(Color color) {
    if (color == Color::Black) {
        return 1;
    }
    return 0;
}

void initialize_tournament_budget(TournamentBudget *manager) {
    int index;

    if (manager == NULL) {
        return;
    }

    for (index = 0; index < 2; ++index) {
        manager->remainingMs[index] = AI_TOURNAMENT_TOTAL_MS;
        manager->poolMs[index] = 0;
    }
}

int tournament_budget_ms(const TournamentBudget *manager, Color color) {
    int index;
    int remainingMs;
    int availableMs;
    int bonusMs;
    int budgetMs;

    if (manager == NULL || (color != Color::White && color != Color::Black)) {
        return AI_TOURNAMENT_BASE_MS;
    }

    index = ai_color_time_index(color);
    remainingMs = manager->remainingMs[index];
    if (remainingMs <= AI_MIN_MOVE_BUDGET_MS) {
        return AI_MIN_MOVE_BUDGET_MS;
    }
    availableMs = remainingMs > AI_TOURNAMENT_RESERVE_MS ? remainingMs - AI_TOURNAMENT_RESERVE_MS : remainingMs;
    bonusMs = manager->poolMs[index] / 4;
    bonusMs = ai_clamp_int(bonusMs, 0, AI_TOURNAMENT_MAX_EXTRA_MS);

    budgetMs = AI_TOURNAMENT_BASE_MS + bonusMs;
    budgetMs = ai_clamp_int(budgetMs, AI_MIN_MOVE_BUDGET_MS, AI_TOURNAMENT_MAX_MS);
    if (availableMs > 0 && budgetMs > availableMs) {
        budgetMs = ai_clamp_int(availableMs, AI_MIN_MOVE_BUDGET_MS, AI_TOURNAMENT_MAX_MS);
    }
    return budgetMs;
}

int tournament_expired(const TournamentBudget *manager, Color color) {
    int index;

    if (manager == NULL || (color != Color::White && color != Color::Black)) {
        return 0;
    }

    index = ai_color_time_index(color);
    return manager->remainingMs[index] <= 0;
}

void charge_tournament_budget(TournamentBudget *manager, Color color, int budgetMs, int elapsedMs) {
    int index;
    int64_t poolMs;

    if (manager == NULL || (color != Color::White && color != Color::Black)) {
        return;
    }

    if (budgetMs <= 0) {
        budgetMs = AI_TOURNAMENT_BASE_MS;
    }
    if (elapsedMs < 0) {
        elapsedMs = 0;
    }

    index = ai_color_time_index(color);
    if (manager->remainingMs[index] > elapsedMs) {
        manager->remainingMs[index] -= elapsedMs;
    } else {
        manager->remainingMs[index] = 0;
    }

    poolMs = manager->poolMs[index];
    if (elapsedMs < budgetMs) {
        poolMs += budgetMs - elapsedMs;
    } else {
        poolMs -= elapsedMs - budgetMs;
    }
    // Saturation follows wide integer arithmetic, preserving the existing cap
    // even when a configured budget would overflow a signed int.
    manager->poolMs[index] = poolMs < 0                           ? 0
                             : poolMs > AI_TOURNAMENT_POOL_CAP_MS ? AI_TOURNAMENT_POOL_CAP_MS
                                                                  : int(poolMs);
}

} // namespace ac
