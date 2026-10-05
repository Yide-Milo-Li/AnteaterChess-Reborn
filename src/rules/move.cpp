#include "anteater/rules.h"

#include <stddef.h>

static void initializeMoveArrays(AcMove *move) {
    int i;

    for (i = 0; i < AC_MAX_CHAIN; ++i) {
        move->path[i] = ac_create_position(-1, -1); /* (-1,-1) is an invalid position */
        move->captures[i].pos = ac_create_position(-1, -1);
        move->captures[i].piece = ac_create_piece(AC_EMPTY_PIECE, AC_EMPTY_COLOR);
    }
}

AcMove ac_create_move(AcSquare from, AcSquare to, AcPiece piece) {
    AcMove move;

    move.from = from;
    move.to = to;
    move.movedPiece = piece;
    move.pathLength = 0;
    move.captureCount = 0;
    move.specialType = AC_NO_SPECIAL_MOVE;
    initializeMoveArrays(&move);
    return move;
}

void ac_add_capture(AcMove *move, AcSquare pos, AcPiece piece) {
    if (move == NULL || move->captureCount >= AC_MAX_CHAIN) {
        return;
    }

    move->captures[move->captureCount].pos = pos;
    move->captures[move->captureCount].piece = piece;
    ++move->captureCount;
}

void ac_add_path_step(AcMove *move, AcSquare pos) {
    if (move == NULL || move->pathLength >= AC_MAX_CHAIN) {
        return;
    }

    move->path[move->pathLength] = pos;
    ++move->pathLength;
}

void ac_set_special_move(AcMove *move, AcSpecialMove type) {
    if (move == NULL) {
        return;
    }

    move->specialType = type;
}

int ac_is_promotion_special_move(AcSpecialMove type) {
    return type == AC_PROMOTION_QUEEN || type == AC_PROMOTION_ROOK || type == AC_PROMOTION_BISHOP ||
           type == AC_PROMOTION_KNIGHT;
}
