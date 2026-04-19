#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/move.h"
#include "core/movelist.h"
#include "log/log.h"

static GameState create_log_state(void) {
    GameState state;

    memset(&state, 0, sizeof(state));
    initMoveList(&state.moveHistory);
    state.result = RESULT_WHITE_WIN;
    return state;
}

static void read_log_file(char *buffer, size_t size) {
    FILE *file = fopen("game.log", "r");
    size_t bytesRead;

    assert(file != NULL);
    bytesRead = fread(buffer, 1, size - 1, file);
    buffer[bytesRead] = '\0';
    fclose(file);
}

static void test_log_lifecycle_and_history_rebuild(void) {
    GameConfig config;
    GameState state = create_log_state();
    Move move;
    char buffer[2048];

    remove("game.log");
    initDefaultGameConfig(&config);

    move = createMove(createPosition(6, 4), createPosition(5, 4), createPiece(ANT, WHITE));
    addMove(&state.moveHistory, move);

    assert(initLog(&config) == 1);
    assert(logGameStart(&config) == 1);
    assert(logMove(&state, move) == 1);
    assert(rebuildLogFromHistory(&state) == 1);
    assert(logGameEnd(&state) == 1);
    closeLog();

    read_log_file(buffer, sizeof(buffer));
    assert(strstr(buffer, "Anteater Chess Game Started") != NULL);
    assert(strstr(buffer, "Mode: Human vs Human") != NULL);
    assert(strstr(buffer, "Move: White Ant E2 -> E3") != NULL);
    assert(strstr(buffer, "Rebuilding Log From History") != NULL);
    assert(strstr(buffer, "Game Over: White Win") != NULL);
}

int main(void) {
    test_log_lifecycle_and_history_rebuild();
    return 0;
}
