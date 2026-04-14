#ifndef CHESS_CORE_PIECE_H
#define CHESS_CORE_PIECE_H

typedef enum {
    ANT,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING,
    ANTEATER,
    EMPTY_PIECE
} PieceType;

typedef enum {
    WHITE,
    BLACK,
    EMPTY_COLOR
} Color;

typedef struct {
    PieceType type;
    Color color;
} Piece;

Piece createPiece(PieceType type, Color color);
int isSameColor(Piece a, Piece b);
char getPieceSymbol(Piece piece);
int isValidPiece(Piece piece);

#endif
