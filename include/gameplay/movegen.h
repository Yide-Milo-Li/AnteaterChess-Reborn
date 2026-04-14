#ifndef CHESS_GAMEPLAY_MOVEGEN_H
#define CHESS_GAMEPLAY_MOVEGEN_H

#include "core/gamestate.h"
#include "core/movelist.h"
#include "core/position.h"

int generateMoves(const GameState *state, MoveList *list);
int generateLegalMoves(const GameState *state, MoveList *list);
int generateLegalMovesForPosition(const GameState *state, Position from, MoveList *list);

#endif
