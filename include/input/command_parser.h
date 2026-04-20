#ifndef CHESS_INPUT_COMMAND_PARSER_H
#define CHESS_INPUT_COMMAND_PARSER_H

#include "input/command.h"

/* Parse two GUI-style move fields into one Command. Field parsing is tolerant
 * of case and leading/trailing whitespace, but not of malformed coordinates. */
int parseMoveCommand(const char *fromText, const char *toText, Command *cmd);

#endif
