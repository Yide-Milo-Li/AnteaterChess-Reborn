#ifndef CHESS_GAMEPLAY_VALIDATION_H
#define CHESS_GAMEPLAY_VALIDATION_H

#include "core/gamestate.h"
#include "core/move.h"
#include "core/position.h"

typedef enum {
    SELECT_VALID,
    SELECT_EMPTY,
    SELECT_OPPONENT_PIECE,
    SELECT_OUT_OF_BOUNDS
} SelectionResult;

int validateMove(const GameState *state, Move move);
SelectionResult validateSelection(const GameState *state, Position pos);

#endif
