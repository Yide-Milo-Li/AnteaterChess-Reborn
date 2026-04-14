#ifndef CHESS_TURN_TURN_TIMER_H
#define CHESS_TURN_TURN_TIMER_H

#include "core/gamestate.h"

int initTurnTimer(GameState *state);
int updateTurnTimer(GameState *state);
int isTimeUp(const GameState *state);
int getRemainingTime(const GameState *state, Color playerColor);
int resetTurnTimer(GameState *state);

#endif
