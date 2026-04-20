#ifndef CHESS_CLI_RENDERER_H
#define CHESS_CLI_RENDERER_H

#include "core/gamestate.h"

int cliRenderBoard(const GameState *state);
int cliDisplayGameStatus(const GameState *state);
int cliDisplayTurn(Color turn);

#endif
