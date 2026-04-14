#ifndef CHESS_CORE_PLAYER_H
#define CHESS_CORE_PLAYER_H

#include "core/piece.h"

typedef enum {
    HUMAN,
    AI
} PlayerType;

typedef struct {
    PlayerType type;
    Color color;
} Player;

Player createPlayer(Color color, PlayerType type);
Color getPlayerColor(Player player);
int isAIPlayer(Player player);
int isHumanPlayer(Player player);

#endif
