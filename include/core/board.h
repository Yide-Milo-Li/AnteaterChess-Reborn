#ifndef CHESS_CORE_BOARD_H
#define CHESS_CORE_BOARD_H

#include "core/piece.h"
#include "core/position.h"

#define ROWS 8
#define COLS 10

typedef struct {
    Piece cells[ROWS][COLS];
} Board;

void initBoard(Board *board);
Piece getPiece(const Board *board, Position pos);
void setPiece(Board *board, Position pos, Piece piece);
void removePiece(Board *board, Position pos);

#endif
