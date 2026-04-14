#ifndef CHESS_INPUT_COMMAND_H
#define CHESS_INPUT_COMMAND_H

#include "core/position.h"

typedef enum {
    CMD_MOVE,
    CMD_UNDO,
    CMD_EXIT,
    CMD_BACK,
    CMD_NEW_GAME,
    CMD_INVALID
} CommandType;

typedef struct {
    CommandType type;
    Position from;
    Position to;
} Command;

int createMoveCommand(Command *cmd, Position from, Position to);
int createSimpleCommand(Command *cmd, CommandType type);

#endif
