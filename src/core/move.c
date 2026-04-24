#include "core/move.h"

#include <stddef.h>

static void initializeMoveArrays(Move *move) {
    int i;

    for (i = 0; i < MAX_CHAIN; ++i) {
        move->path[i] = createPosition(-1, -1); /* (-1,-1) is an invalid position */
        move->captures[i].pos = createPosition(-1, -1);
        move->captures[i].piece = createPiece(EMPTY_PIECE, EMPTY_COLOR);
    }
}

Move createMove(Position from, Position to, Piece piece) {
    Move move;

    move.from = from;
    move.to = to;
    move.movedPiece = piece;
    move.pathLength = 0;
    move.captureCount = 0;
    move.specialType = NO_SPECIAL_MOVE;
    initializeMoveArrays(&move);
    return move;
}

void addCapture(Move *move, Position pos, Piece piece) {
    if (move == NULL || move->captureCount >= MAX_CHAIN) {
        return;
    }

    move->captures[move->captureCount].pos = pos;
    move->captures[move->captureCount].piece = piece;
    ++move->captureCount;
}

void addPathStep(Move *move, Position pos) {
    if (move == NULL || move->pathLength >= MAX_CHAIN) {
        return;
    }

    move->path[move->pathLength] = pos;
    ++move->pathLength;
}

void setSpecialMove(Move *move, SpecialMove type) {
    if (move == NULL) {
        return;
    }

    move->specialType = type;
}

int isPromotionSpecialMove(SpecialMove type) {
    return type == PROMOTION_QUEEN
        || type == PROMOTION_ROOK
        || type == PROMOTION_BISHOP
        || type == PROMOTION_KNIGHT;
}
