#ifndef CHESS_AI_TOURNAMENT_AI_H
#define CHESS_AI_TOURNAMENT_AI_H

#include "core/gamestate.h"
#include "core/move.h"

int generateTournamentAIMoveWithBudget(const GameState *state,
                                       Move *move,
                                       int budgetMs);

#endif
