#include <stdio.h>

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "system/controller.h"

/*
 * Alignment assumptions for future extensions:
 * - main only wires together public contracts and should not depend on controller internals.
 * - controller.h is the truth source for the system loop entrypoint.
 * - Initial system state comes from GameState initialization, not hidden FSM globals.
 */

/* Boot one default game state and hand control to the public controller loop. */
int main(void) {
    GameConfig config;
    GameState state;
    int result;

    initDefaultGameConfig(&config);
    initGameState(&state, &config);

    printf("[AnteaterChess] Phase E controller starting in state %d.\n", (int)state.systemState);
    result = runGameLoop(&state);
    printf("[AnteaterChess] Controller stopped in state %d with rc=%d.\n", (int)state.systemState, result);

    return result;
}
