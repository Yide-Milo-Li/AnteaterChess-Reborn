#ifndef CHESS_INPUT_COMMAND_PARSER_H
#define CHESS_INPUT_COMMAND_PARSER_H

#include "input/command.h"

int parseMoveCommand(const char *fromText, const char *toText, Command *cmd);

#endif
