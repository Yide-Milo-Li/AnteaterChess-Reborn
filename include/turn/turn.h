#ifndef CHESS_TURN_TURN_H
#define CHESS_TURN_TURN_H

#include "core/gamestate.h"

int switchTurn(GameState *state);
Color getCurrentTurn(const GameState *state);
int isPlayerTurn(const GameState *state, Color playerColor);
int restoreTurnAfterUndo(GameState *state);

#endif
