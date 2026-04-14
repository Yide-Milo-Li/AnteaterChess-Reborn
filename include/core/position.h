#ifndef CHESS_CORE_POSITION_H
#define CHESS_CORE_POSITION_H

typedef struct {
    int row;
    int col;
} Position;

Position createPosition(int row, int col);
int isValidPosition(Position pos);
Position parsePosition(const char *input);
int positionEqual(Position a, Position b);

#endif
