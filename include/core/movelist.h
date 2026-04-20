#ifndef CHESS_CORE_MOVELIST_H
#define CHESS_CORE_MOVELIST_H

#include "core/move.h"

/* Increased to 2048 so longer sessions and analysis paths do not hit history capacity as early. */
#define MAX_MOVES 2048

typedef struct {
    Move moves[MAX_MOVES];
    int count;
} MoveList;

void initMoveList(MoveList *list);
int addMove(MoveList *list, Move move);
int removeLastMove(MoveList *list);
Move *getMove(MoveList *list, int index);
int getMoveCount(MoveList *list);

#endif
