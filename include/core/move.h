#ifndef CHESS_CORE_MOVE_H
#define CHESS_CORE_MOVE_H

#include "core/piece.h"
#include "core/position.h"

#define MAX_CHAIN 10

typedef enum {
    NO_SPECIAL_MOVE,
    CASTLING_KINGSIDE,
    CASTLING_QUEENSIDE,
    EN_PASSANT,
    PROMOTION_QUEEN,
    PROMOTION_ROOK,
    PROMOTION_BISHOP,
    PROMOTION_KNIGHT,
    PROMOTION_ANTEATER,
    ANTEATER_CAPTURE
} SpecialMove;

typedef struct {
    Position pos;
    Piece piece;
} CaptureRecord;

typedef struct {
    Position from;
    Position to;
    Piece movedPiece;
    Position path[MAX_CHAIN];
    int pathLength;
    CaptureRecord captures[MAX_CHAIN];
    int captureCount;
    SpecialMove specialType;
} Move;

Move createMove(Position from, Position to, Piece piece);
void addCapture(Move *move, Position pos, Piece piece);
void addPathStep(Move *move, Position pos);
void setSpecialMove(Move *move, SpecialMove type);
int isPromotionSpecialMove(SpecialMove type);

#endif
