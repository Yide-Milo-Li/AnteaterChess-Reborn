#ifndef CHESS_INPUT_INPUT_H
#define CHESS_INPUT_INPUT_H

#include "core/position.h"
#include "input/command.h"

typedef enum {
    INPUT_SELECT_PIECE,
    INPUT_SELECT_DESTINATION,
    INPUT_CANCEL
} InputType;

int getMoveInput(Command *cmd);
int getBoardInput(Position *pos, InputType *type);
int getMenuSelection(int *selection);

#endif
