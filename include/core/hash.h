#ifndef CHESS_CORE_HASH_H
#define CHESS_CORE_HASH_H

#include <stdint.h>

#include "core/gamestate.h"
#include "core/move.h"

#define TT_SIZE (1U << 20)

#define HASH_CASTLE_WHITE_KINGSIDE (1U << 0)
#define HASH_CASTLE_WHITE_QUEENSIDE (1U << 1)
#define HASH_CASTLE_BLACK_KINGSIDE (1U << 2)
#define HASH_CASTLE_BLACK_QUEENSIDE (1U << 3)
#define HASH_NO_EN_PASSANT_FILE (-1)

typedef struct {
    uint64_t value;
    unsigned char castlingRights;
    int enPassantFile;
} HashState;

void initZobrist(void);
uint64_t computeHash(const GameState *state);
uint64_t updateHashMove(uint64_t hash, Move move);
int deriveHashState(const GameState *state, HashState *out);
int advanceHashState(const GameState *after, Move move, const HashState *previous, HashState *next);

#endif
