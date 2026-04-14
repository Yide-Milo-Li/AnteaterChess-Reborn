#ifndef CHESS_CORE_MOVELIST_H
#define CHESS_CORE_MOVELIST_H

#include "core/move.h"

#define MAX_MOVES 1024

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
