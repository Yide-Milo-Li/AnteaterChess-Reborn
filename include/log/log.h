#ifndef CHESS_LOG_LOG_H
#define CHESS_LOG_LOG_H

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/move.h"

int initLog(const GameConfig *config);
int logGameStart(const GameConfig *config);
int logMove(const GameState *state, Move move);
int rebuildLogFromHistory(const GameState *state);
int logGameEnd(const GameState *state);
void closeLog(void);

#endif
