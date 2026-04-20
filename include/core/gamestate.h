#ifndef CHESS_CORE_GAMESTATE_H
#define CHESS_CORE_GAMESTATE_H

#include <stdint.h>

#include "core/board.h"
#include "core/gameconfig.h"
#include "core/movelist.h"
#include "core/player.h"
#include "system/system_state.h"

typedef enum {
    RESULT_NONE,
    RESULT_WHITE_WIN,
    RESULT_BLACK_WIN,
    RESULT_DRAW,
    RESULT_TERMINATED_BY_USER
} GameResult;

typedef struct {
    Board board;
    Player players[2];
    Color currentTurn;
    int moveCount;
    int gameOver;
    GameResult result;
    SystemState systemState;
    GameConfig config;
    MoveList moveHistory;
    uint64_t hash;
} GameState;

void initGameState(GameState *state, const GameConfig *config);
int isGameOver(const GameState *state);
void setGameOver(GameState *state);
Player *getCurrentPlayer(GameState *state);
MoveList *getMoveHistory(GameState *state);
int addMoveToHistory(GameState *state, Move move);
void setGameResult(GameState *state, GameResult result);
GameResult getGameResult(GameState *state);
int removeLastMoveFromHistory(GameState *state);

#endif
