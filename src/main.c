#include "ai/ai.h"
#include "time/clock.h"
#include "ui/gui.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    AITimeManager timeManager;
    int initialized;
} MainAIContext;

static AIDifficulty main_current_ai_difficulty(const GameState *state) {
    if (state == NULL) {
        return DIFFICULTY_NONE;
    }

    return state->currentTurn == WHITE
        ? state->config.aiDifficultyWhite
        : state->config.aiDifficultyBlack;
}

static void main_reset_ai_context(void *context, const GameConfig *config) {
    MainAIContext *aiContext = (MainAIContext *)context;

    (void)config;
    if (aiContext == NULL) {
        return;
    }

    initAITimeManager(&aiContext->timeManager);
    aiContext->initialized = 1;
}

static int main_generate_ai_move(const GameState *state, Move *move, void *context) {
    MainAIContext *aiContext = (MainAIContext *)context;
    int budgetMs;
    int result;
    int elapsedMs;
    int64_t startMs;
    int64_t endMs;

    if (state != NULL && main_current_ai_difficulty(state) == DIFFICULTY_TOURNAMENT) {
        if (aiContext == NULL) {
            return 1;
        }
        if (!aiContext->initialized) {
            initAITimeManager(&aiContext->timeManager);
            aiContext->initialized = 1;
        }

        budgetMs = getAITournamentBudgetMs(&aiContext->timeManager, state->currentTurn);
        startMs = 0;
        endMs = 0;
        if (getMonotonicMilliseconds(&startMs) != 0) {
            startMs = 0;
        }
        result = generateAIMoveWithBudget(state, move, budgetMs);
        if (getMonotonicMilliseconds(&endMs) == 0 && endMs >= startMs) {
            elapsedMs = (endMs - startMs > INT32_MAX) ? INT32_MAX : (int)(endMs - startMs);
        } else {
            elapsedMs = budgetMs;
        }
        updateAITournamentTime(&aiContext->timeManager, state->currentTurn, budgetMs, elapsedMs);
        return result;
    }

    return generateAIMove(state, move);
}

static int main_generate_hint_move(const GameState *state, Move *move, void *context) {
    (void)context;
    return generateHintMove(state, move);
}

int main(int argc, char **argv) {
    MainAIContext aiContext;
    Gui *gui = gui_create(&argc, &argv);

    if (gui == NULL) {
        return 1;
    }

    initAITimeManager(&aiContext.timeManager);
    aiContext.initialized = 1;

    gui_set_move_provider(gui, main_generate_ai_move, &aiContext);
    gui_set_move_provider_reset(gui, main_reset_ai_context, &aiContext);
    gui_set_hint_provider(gui, main_generate_hint_move, NULL);
    gui_run(gui);
    gui_destroy(gui);
    return 0;
}
