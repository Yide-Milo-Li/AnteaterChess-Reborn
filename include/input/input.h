#ifndef CHESS_INPUT_INPUT_H
#define CHESS_INPUT_INPUT_H

#include "core/position.h"
#include "input/command.h"

typedef enum {
    INPUT_SELECT_PIECE,
    INPUT_SELECT_DESTINATION,
    INPUT_CANCEL
} InputType;

/* Read one CLI move command for tests or terminal fallback input. */
int getMoveInput(Command *cmd);
/* Read one raw coordinate token without assigning source/destination semantics. */
int getBoardInput(Position *pos, InputType *type);
/* Read one raw integer menu selection from the active input stream. */
int getMenuSelection(int *selection);

#endif
