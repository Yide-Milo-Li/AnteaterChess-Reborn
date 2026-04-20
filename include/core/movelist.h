#ifndef CHESS_CORE_MOVELIST_H
#define CHESS_CORE_MOVELIST_H

#include "core/move.h"

/* Reduced back to 1024 because larger values currently put too much pressure on stack-heavy search and analysis paths. */
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
