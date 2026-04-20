#ifndef CHESS_CLI_MENU_H
#define CHESS_CLI_MENU_H

#include "core/gameconfig.h"
#include "core/gamestate.h"

int cliGetMainMenuSelection(int *selection);
int cliGetGameModeSelection(int *selection);
int cliGetGameSetupConfig(GameConfig *config);
int cliShowEndGameMenu(const GameState *state, int *selection);

#endif
