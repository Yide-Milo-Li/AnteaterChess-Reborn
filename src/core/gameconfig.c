#include "core/gameconfig.h"

#include <stddef.h>

void initDefaultGameConfig(GameConfig *config) {
    if (config == NULL) {
        return;
    }

    config->mode = MODE_HUMAN_VS_HUMAN;
    config->playerColor = WHITE;
    config->aiDifficultyWhite = DIFFICULTY_NONE;
    config->aiDifficultyBlack = DIFFICULTY_NONE;
    config->timerEnabled = 0;
    config->aiTimeLimit = 0;
    config->initialTimeSeconds = 0;
}
