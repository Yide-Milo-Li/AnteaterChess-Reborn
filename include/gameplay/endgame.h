#ifndef CHESS_GAMEPLAY_ENDGAME_H
#define CHESS_GAMEPLAY_ENDGAME_H

#include "core/gamestate.h"

int isInCheck(const GameState *state, Color color);
int isCheckmate(const GameState *state, Color color);
int isStalemate(const GameState *state, Color color);
int isInsufficientMaterial(const GameState *state);
int isThreefoldRepetition(const GameState *state);
int detectGameResult(GameState *state);

#endif
