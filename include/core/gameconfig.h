#ifndef CHESS_CORE_GAMECONFIG_H
#define CHESS_CORE_GAMECONFIG_H

#include "core/piece.h"

typedef enum {
    MODE_HUMAN_VS_HUMAN,
    MODE_HUMAN_VS_COMPUTER,
    MODE_COMPUTER_VS_COMPUTER
} GameMode;

typedef enum {
    DIFFICULTY_NONE,
    DIFFICULTY_EASY,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD,
    DIFFICULTY_EXPERIMENTAL
} AIDifficulty;

typedef struct {
    GameMode mode;
    Color playerColor;
    AIDifficulty aiDifficultyWhite;
    AIDifficulty aiDifficultyBlack;
    int timerEnabled;
    int aiTimeLimit;
    int initialTimeSeconds;
} GameConfig;

void initDefaultGameConfig(GameConfig *config);
void initGameConfigForMode(GameConfig *config, GameMode mode);
int getDefaultAITimeBudgetMs(AIDifficulty difficulty);
int getAITimeBudgetMs(const GameConfig *config, AIDifficulty difficulty);
int getRequiredAITurnTimerSeconds(const GameConfig *config);
int isAITurnTimerSettingValid(const GameConfig *config);

#endif
