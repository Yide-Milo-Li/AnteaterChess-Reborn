#include "internal.h"
int ac_ai_clamp_int(int value, int minValue, int maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

int ac_ai_color_time_index(AcColor color) {
    if (color == AC_BLACK) {
        return 1;
    }
    return 0;
}

void ac_init_ai_time_manager(AcAITimeManager *manager) {
    int index;

    if (manager == NULL) {
        return;
    }

    for (index = 0; index < 2; ++index) {
        manager->remainingMs[index] = AI_TOURNAMENT_TOTAL_MS;
        manager->poolMs[index] = 0;
    }
}

int ac_get_ai_tournament_budget_ms(AcAITimeManager *manager, AcColor color) {
    int index;
    int remainingMs;
    int availableMs;
    int bonusMs;
    int budgetMs;

    if (manager == NULL || (color != AC_WHITE && color != AC_BLACK)) {
        return AI_TOURNAMENT_BASE_MS;
    }

    index = ac_ai_color_time_index(color);
    remainingMs = manager->remainingMs[index];
    if (remainingMs <= AI_MIN_MOVE_BUDGET_MS) {
        return AI_MIN_MOVE_BUDGET_MS;
    }
    availableMs = remainingMs > AI_TOURNAMENT_RESERVE_MS ? remainingMs - AI_TOURNAMENT_RESERVE_MS : remainingMs;
    bonusMs = manager->poolMs[index] / 4;
    bonusMs = ac_ai_clamp_int(bonusMs, 0, AI_TOURNAMENT_MAX_EXTRA_MS);

    budgetMs = AI_TOURNAMENT_BASE_MS + bonusMs;
    budgetMs = ac_ai_clamp_int(budgetMs, AI_MIN_MOVE_BUDGET_MS, AI_TOURNAMENT_MAX_MS);
    if (availableMs > 0 && budgetMs > availableMs) {
        budgetMs = ac_ai_clamp_int(availableMs, AI_MIN_MOVE_BUDGET_MS, AI_TOURNAMENT_MAX_MS);
    }
    return budgetMs;
}

int ac_is_ai_tournament_time_expired(const AcAITimeManager *manager, AcColor color) {
    int index;

    if (manager == NULL || (color != AC_WHITE && color != AC_BLACK)) {
        return 0;
    }

    index = ac_ai_color_time_index(color);
    return manager->remainingMs[index] <= 0;
}

void ac_update_ai_tournament_time(AcAITimeManager *manager, AcColor color, int budgetMs, int elapsedMs) {
    int index;
    int poolMs;

    if (manager == NULL || (color != AC_WHITE && color != AC_BLACK)) {
        return;
    }

    if (budgetMs <= 0) {
        budgetMs = AI_TOURNAMENT_BASE_MS;
    }
    if (elapsedMs < 0) {
        elapsedMs = 0;
    }

    index = ac_ai_color_time_index(color);
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
    manager->poolMs[index] = ac_ai_clamp_int(poolMs, 0, AI_TOURNAMENT_POOL_CAP_MS);
}
