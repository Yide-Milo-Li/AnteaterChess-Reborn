#include "anteater/rules.hpp"

#include <stddef.h>

namespace ac {

static void initializeMoveArrays(Move *move) {
    int i;

    for (i = 0; i < MaxChain; ++i) {
        move->path[i] = create_position(-1, -1); /* (-1,-1) is an invalid position */
        move->captures[i].pos = create_position(-1, -1);
        move->captures[i].piece = create_piece(PieceType::Empty, Color::Empty);
    }
}

Move create_move(Square from, Square to, Piece piece) {
    Move move;

    move.from = from;
    move.to = to;
    move.movedPiece = piece;
    move.pathLength = 0;
    move.captureCount = 0;
    move.specialType = SpecialMove::None;
    initializeMoveArrays(&move);
    return move;
}

void add_capture(Move *move, Square pos, Piece piece) {
    if (move == NULL || move->captureCount >= MaxChain) {
        return;
    }

    move->captures[move->captureCount].pos = pos;
    move->captures[move->captureCount].piece = piece;
    ++move->captureCount;
}

void add_path_step(Move *move, Square pos) {
    if (move == NULL || move->pathLength >= MaxChain) {
        return;
    }

    move->path[move->pathLength] = pos;
    ++move->pathLength;
}

void set_special_move(Move *move, SpecialMove type) {
    if (move == NULL) {
        return;
    }

    move->specialType = type;
}

int is_promotion_special_move(SpecialMove type) {
    return type == SpecialMove::PromotionQueen || type == SpecialMove::PromotionRook ||
           type == SpecialMove::PromotionBishop || type == SpecialMove::PromotionKnight;
}

} // namespace ac
