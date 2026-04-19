#include "turn/turn.h"

#include <stddef.h>

/* Switch from the active player to the other player */
int switchTurn(GameState *state) {
    if (state == NULL) {
        return 1;
    }

    if (state->currentTurn == WHITE) {
        state->currentTurn = BLACK;
        return 0;
    }

    if (state->currentTurn == BLACK) {
        state->currentTurn = WHITE;
        return 0;
    }

    return 1;
}

/* Return the color whose turn it currently is */
Color getCurrentTurn(const GameState *state) {
    if (state == NULL) {
        return EMPTY_COLOR;
    }

    if (state->currentTurn == WHITE) {
        return WHITE;
    }

    if (state->currentTurn == BLACK) {
        return BLACK;
    }

    return EMPTY_COLOR;
}

/* Check whether the requested player color matches the active turn. 
 * Returns 1 if it is the specfied player's turn and 0 if not */
int isPlayerTurn(const GameState *state, Color playerColor) {
    Color currentTurn;

    if (state == NULL) {
        return 0;
    }

    if (playerColor != WHITE && playerColor != BLACK) {
        return 0;
    }

    currentTurn = getCurrentTurn(state);
    if (currentTurn == EMPTY_COLOR) {
        return 0;
    }

    if (currentTurn == playerColor) {
        return 1;
    }

    return 0;
}

/* After an undo, the remaining move count tells us whose turn should be next.
 * White starts the game, so an even number of completed moves means white's
 * turn and an odd number means black's turn. */
int restoreTurnAfterUndo(GameState *state) {
    if (state == NULL) {
        return 1;
    }

    if (state->moveHistory.count < 0) {
        return 1;
    }

    if ((state->moveHistory.count % 2) == 0) {
        state->currentTurn = WHITE;
    } else {
        state->currentTurn = BLACK;
    }

    return 0;
}
