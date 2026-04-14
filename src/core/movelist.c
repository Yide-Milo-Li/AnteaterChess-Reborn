#include "core/movelist.h"

#include <stddef.h>

void initMoveList(MoveList *list) {
    if (list == NULL) {
        return;
    }

    list->count = 0;
}

int addMove(MoveList *list, Move move) {
    if (list == NULL || list->count >= MAX_MOVES) {
        return 1;
    }

    list->moves[list->count] = move;
    ++list->count;
    return 0;
}

int removeLastMove(MoveList *list) {
    if (list == NULL || list->count <= 0) {
        return 1;
    }

    --list->count;
    return 0;
}

Move *getMove(MoveList *list, int index) {
    if (list == NULL || index < 0 || index >= list->count) {
        return NULL;
    }

    return &list->moves[index];
}

int getMoveCount(MoveList *list) {
    if (list == NULL) {
        return 0;
    }

    return list->count;
}
