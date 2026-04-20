#ifndef CHESS_CLI_GAMEPLAY_H
#define CHESS_CLI_GAMEPLAY_H

#include "input/command.h"

int cliGetGameplayAction(int *selection);
int cliGetMoveCommand(Command *cmd);
int cliShowMoveFormatHint(void);

#endif
