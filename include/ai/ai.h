#ifndef CHESS_AI_AI_H
#define CHESS_AI_AI_H

#include "core/gamestate.h"
#include "core/move.h"

int generateAIMove(const GameState *state, Move *move);
int generateHintMove(const GameState *state, Move *move);

#endif
