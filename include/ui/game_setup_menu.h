#ifndef CHESS_UI_GAME_SETUP_MENU_H
#define CHESS_UI_GAME_SETUP_MENU_H

#include "core/gameconfig.h"

void initGameSetupConfig(GameMode mode);
void setPlayerColor(Color color);
void setAIDifficultyWhite(AIDifficulty diff);
void setAIDifficultyBlack(AIDifficulty diff);
void setTimerEnabled(int enabled);
void setInitialTimeSeconds(int seconds);
int getGameSetupConfig(GameConfig *config);
void setStartPressed(int val);
int isStartPressed();
void resetStartPressed();

#endif
