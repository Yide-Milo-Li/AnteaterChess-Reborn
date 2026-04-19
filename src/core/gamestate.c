#include "core/gamestate.h"

#include <stddef.h>

/*
 * Alignment assumptions for future extensions:
 * - This file owns GameState-level consistency helpers, not gameplay rules.
 * - setGameResult() is the authoritative path for keeping result/gameOver aligned.
 * - setGameOver() remains a compatibility helper for user-driven termination only.
 * - Callers may read GameState immediately after initialization, so init must reset every field.
 */

/* Set up a brand-new game state */
void initGameState(GameState *state, const GameConfig *config) {
    GameConfig effectiveConfig;
    PlayerType whiteType;
    PlayerType blackType;
    Color humanColor;

    /* Check for empty state (invalid) */
    if (state == NULL) {
        return;
    }

    /* Start from known defaults, then overwrite them if the caller provided
     * a custom config */
    initDefaultGameConfig(&effectiveConfig);
    if (config != NULL) {
        effectiveConfig = *config;
    }

    /* Assume both sides are human unless the game mode says otherwise */
    whiteType = HUMAN;
    blackType = HUMAN;

    /* In human-vs-computer mode, playerColor selects the human side. Falling
     * back to white keeps initialization predictable if the config was only
     * partially filled in. */
    humanColor = WHITE;
    if (effectiveConfig.playerColor == BLACK) {
        humanColor = BLACK;
    }

    /* Determine the playerColor based on mode selection */
    if (effectiveConfig.mode == MODE_COMPUTER_VS_COMPUTER) {
        whiteType = AI;
        blackType = AI;
    } else if (effectiveConfig.mode == MODE_HUMAN_VS_COMPUTER) {
        if (humanColor == WHITE) {
            whiteType = HUMAN;
            blackType = AI;
        } else {
            whiteType = AI;
            blackType = HUMAN;
        }
    }

    /* GameState is the project-wide runtime snapshot, so initialization resets
     * every field that other modules may read before play begins */
    initBoard(&state->board);
    state->config = effectiveConfig;

    /* The players array is stored by color: index 0 is white, index 1 is black */
    state->players[WHITE] = createPlayer(WHITE, whiteType);
    state->players[BLACK] = createPlayer(BLACK, blackType);

    /* White always opens the game */
    state->currentTurn = WHITE;
    state->moveCount = 0;
    state->gameOver = 0;
    state->result = RESULT_NONE;
    state->systemState = INIT_STATE;

    /* No moves have happened yet, so initialize move history */
    initMoveList(&state->moveHistory);

    /* Initialized state starts with a neutral placeholder value for hash */
    state->hash = 0;
}

/* Report whether the state is currently marked as terminal. */
int isGameOver(const GameState *state) {
    /* Check is state pointer is null */
    if (state == NULL) {
        return 0;
    }

    /* Return 0 while the game is still active */
    if (state->gameOver == 0) {
        return 0;
    }

    /* Return 1 after game over*/
    return 1;
}

/* Mark the game as user-terminated while preserving result/gameOver consistency. */
void setGameOver(GameState *state) {
    if (state == NULL) {
        return;
    }

    /* setGameResult is the single point that keeps gameOver and result
     * synchronized for terminal states. */
    setGameResult(state, RESULT_TERMINATED_BY_USER);
}

/* Return the player whose color matches currentTurn. */
Player *getCurrentPlayer(GameState *state) {
    if (state == NULL) {
        return NULL;
    }

    /* currentTurn tells us which slot in the players array is active. */
    if (state->currentTurn == WHITE) {
        return &state->players[WHITE];
    }

    if (state->currentTurn == BLACK) {
        return &state->players[BLACK];
    }

    return NULL;
}

/* Return the move history container owned by the state. */
MoveList *getMoveHistory(GameState *state) {
    if (state == NULL) {
        return NULL;
    }

    return &state->moveHistory;
}

/* Append one move to history and keep moveCount in sync. */
int addMoveToHistory(GameState *state, Move move) {
    if (state == NULL) {
        return 1;
    }

    /* Perform operation and check for failure */
    if (addMove(&state->moveHistory, move) != 0) {
        return 1;
    }

    /* Update moveCount +1 */
    ++state->moveCount;
    return 0;
}

/* Update the terminal result and derive the matching gameOver flag. */
void setGameResult(GameState *state, GameResult result) {
    if (state == NULL) {
        return;
    }

    /* set result flag
     * RESULT_NONE => continues game, otherwise gameOver = 1*/
    state->result = result;
    if (result == RESULT_NONE) {
        state->gameOver = 0;
    } else {
        state->gameOver = 1;
    }
}

/* Return the currently stored game result. */
GameResult getGameResult(GameState *state) {
    if (state == NULL) {
        return RESULT_NONE;
    }

    return state->result;
}

/* Remove the most recent historical move and decrement moveCount if needed. */
int removeLastMoveFromHistory(GameState *state) {
    if (state == NULL) {
        return 1;
    }

    /* Returns 0 if operation is a success otherwise return 1 */
    if (removeLastMove(&state->moveHistory) != 0) {
        return 1;
    }

    if (state->moveCount > 0) {
        --state->moveCount;
    }

    return 0;
}
