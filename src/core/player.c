#include "core/player.h"

/* Creates a Player with input color (black or white) and input type (human or ai) */
Player createPlayer(Color color, PlayerType type) {
    Player player;

    player.type = type;
    player.color = color;
    return player;
}

/* Returns the current player's color */
Color getPlayerColor(Player player) {
    return player.color;
}

/* Returns 1 if player is AI*/
int isAIPlayer(Player player) {
    return player.type == AI;
}

/* Returns 1 if player is HUMAN */
int isHumanPlayer(Player player) {
    return player.type == HUMAN;
}
