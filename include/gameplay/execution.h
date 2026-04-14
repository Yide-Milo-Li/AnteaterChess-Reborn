#ifndef CHESS_GAMEPLAY_EXECUTION_H
#define CHESS_GAMEPLAY_EXECUTION_H

#include "core/gamestate.h"
#include "core/move.h"

int applyMove(GameState *state, Move move);
int undoMove(GameState *state);

#endif
