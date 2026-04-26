#ifndef CHESS_AI_AI_H
#define CHESS_AI_AI_H

#include "core/gamestate.h"
#include "core/move.h"
#include "core/piece.h"

typedef struct {
    int remainingMs[2];
    int poolMs[2];
} AITimeManager;

void initAITimeManager(AITimeManager *manager);
int getAITournamentBudgetMs(AITimeManager *manager, Color color);
int isAITournamentTimeExpired(const AITimeManager *manager, Color color);
void updateAITournamentTime(AITimeManager *manager,
                            Color color,
                            int budgetMs,
                            int elapsedMs);
int generateAIMoveWithBudget(const GameState *state, Move *move, int budgetMs);
int generateAIMove(const GameState *state, Move *move);
int generateHintMove(const GameState *state, Move *move);

#endif
