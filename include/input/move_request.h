#ifndef CHESS_INPUT_MOVE_REQUEST_H
#define CHESS_INPUT_MOVE_REQUEST_H

#include "core/position.h"

typedef enum {
    PROMOTION_CHOICE_NONE,
    PROMOTION_CHOICE_QUEEN,
    PROMOTION_CHOICE_ROOK,
    PROMOTION_CHOICE_BISHOP,
    PROMOTION_CHOICE_KNIGHT
} PromotionChoice;

typedef struct {
    Position from;
    Position to;
    PromotionChoice promotion;
} MoveRequest;

int isValidPromotionChoice(PromotionChoice promotion);
int createMoveRequest(MoveRequest *request, Position from, Position to,
                      PromotionChoice promotion);

#endif
