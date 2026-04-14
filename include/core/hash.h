#ifndef CHESS_CORE_HASH_H
#define CHESS_CORE_HASH_H

#include <stdint.h>

#include "core/gamestate.h"
#include "core/move.h"

#define TT_SIZE (1U << 20)

void initZobrist(void);
uint64_t computeHash(const GameState *state);
uint64_t updateHashMove(uint64_t hash, Move move);

#endif
