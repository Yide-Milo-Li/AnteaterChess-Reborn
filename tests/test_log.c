#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#endif

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/move.h"
#include "log/log.h"
#include "time/clock.h"

/*
 * Alignment assumptions for future extensions:
 * - The log module owns session-local timestamp metadata needed for rebuild.
 * - These tests validate the spec-facing file format rather than controller integration.
 * - Undo correctness is defined as "rewrite from surviving move history without an undo entry."
 */

/* Spin until the elapsed gameplay clock changes or a hard loop cap is reached. */
static int64_t wait_for_elapsed_change(int64_t baseline) {
    time_t deadline = time(NULL) + 3;

    while (time(NULL) <= deadline) {
        int64_t current = getElapsedTimeSeconds();

        if (current != baseline) {
            return current;
        }
    }

    return baseline;
}

/* Build a state with config and player roles that exercise the log formatter. */
static GameState create_log_state(void) {
    GameConfig config;
    GameState state;

    initDefaultGameConfig(&config);
    config.mode = MODE_HUMAN_VS_COMPUTER;
    config.playerColor = WHITE;
    config.aiDifficultyBlack = DIFFICULTY_MEDIUM;
    config.timerEnabled = 1;
    config.aiTimeLimit = 12;
    config.initialTimeSeconds = 30;
    initGameState(&state, &config);
    state.result = RESULT_NONE;
    state.gameOver = 0;
    return state;
}

/* Find the newest timestamped session log under logs/. */
static void find_latest_log_path(char *buffer, size_t size) {
#ifdef _WIN32
    struct _finddata_t fileInfo;
    intptr_t handle;
    char bestName[256];

    bestName[0] = '\0';
    handle = _findfirst("logs\\game_*.log", &fileInfo);
    assert(handle != -1);

    do {
        if (bestName[0] == '\0' || strcmp(fileInfo.name, bestName) > 0) {
            strncpy(bestName, fileInfo.name, sizeof(bestName) - 1);
            bestName[sizeof(bestName) - 1] = '\0';
        }
    } while (_findnext(handle, &fileInfo) == 0);

    _findclose(handle);
    snprintf(buffer, size, "logs/%s", bestName);
#else
    DIR *dir;
    struct dirent *entry;
    char bestName[256];

    bestName[0] = '\0';
    dir = opendir("logs");
    assert(dir != NULL);

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "game_", 5) != 0) {
            continue;
        }

        if (bestName[0] == '\0' || strcmp(entry->d_name, bestName) > 0) {
            strncpy(bestName, entry->d_name, sizeof(bestName) - 1);
            bestName[sizeof(bestName) - 1] = '\0';
        }
    }

    closedir(dir);
    assert(bestName[0] != '\0');
    snprintf(buffer, size, "logs/%s", bestName);
#endif
}

/* Read one full log file into a caller-provided buffer. */
static void read_log_file(const char *path, char *buffer, size_t size) {
    FILE *file = fopen(path, "r");
    size_t bytesRead;

    assert(file != NULL);
    bytesRead = fread(buffer, 1, size - 1, file);
    buffer[bytesRead] = '\0';
    fclose(file);
}

/* Copy one full line identified by a prefix so rebuild output can be compared exactly. */
static void extract_line_by_prefix(const char *buffer, const char *prefix, char *lineBuffer, size_t size) {
    const char *lineStart = strstr(buffer, prefix);
    const char *lineEnd;
    size_t lineLength;

    assert(lineStart != NULL);
    lineEnd = strchr(lineStart, '\n');
    if (lineEnd == NULL) {
        lineEnd = lineStart + strlen(lineStart);
    }

    lineLength = (size_t) (lineEnd - lineStart);
    assert(lineLength + 1 < size);
    memcpy(lineBuffer, lineStart, lineLength);
    lineBuffer[lineLength] = '\0';
}

/* Verify session logging, AI labels, and rebuild-after-undo behavior. */
static void test_log_lifecycle_and_history_rebuild(void) {
    GameState state = create_log_state();
    Move whiteMove;
    Move blackMove;
    char path[256];
    char buffer[4096];
    char firstMoveLine[256];
    int64_t baselineElapsed;

    assert(initClock() == 0);
    assert(initLog(&state.config) == 0);
    assert(logGameStart(&state.config) == 0);

    whiteMove = createMove(createPosition(6, 4), createPosition(5, 4), createPiece(ANT, WHITE));
    assert(addMoveToHistory(&state, whiteMove) == 0);
    assert(logMove(&state, whiteMove) == 0);

    baselineElapsed = getElapsedTimeSeconds();
    assert(wait_for_elapsed_change(baselineElapsed) != baselineElapsed);

    blackMove = createMove(createPosition(1, 4), createPosition(2, 4), createPiece(ANT, BLACK));
    assert(addMoveToHistory(&state, blackMove) == 0);
    assert(logMove(&state, blackMove) == 0);

    find_latest_log_path(path, sizeof(path));
    read_log_file(path, buffer, sizeof(buffer));
    assert(strstr(buffer, "Anteater Chess Game Log") != NULL);
    assert(strstr(buffer, "Mode: Human vs Computer") != NULL);
    assert(strstr(buffer, "Timer Enabled: Yes") != NULL);
    assert(strstr(buffer, "AI Time Limit: 12") != NULL);
    assert(strstr(buffer, "[Move 001] 00:00:00 | White | Ant E2 -> E3") != NULL);
    assert(strstr(buffer, "Black (AI) | Ant E7 -> E6") != NULL);
    extract_line_by_prefix(buffer, "[Move 001]", firstMoveLine, sizeof(firstMoveLine));

    assert(removeLastMoveFromHistory(&state) == 0);
    assert(rebuildLogFromHistory(&state) == 0);
    state.result = RESULT_WHITE_WIN;
    state.gameOver = 1;
    assert(logGameEnd(&state) == 0);
    closeLog();

    read_log_file(path, buffer, sizeof(buffer));
    assert(strstr(buffer, firstMoveLine) != NULL);
    assert(strstr(buffer, "[Move 002]") == NULL);
    assert(strstr(buffer, "Termination Summary:") != NULL);
    assert(strstr(buffer, "Result: White Win") != NULL);
    assert(strstr(buffer, "Total Elapsed Time: ") != NULL);
}

/* Run the persistent log regression suite. */
int main(void) {
    test_log_lifecycle_and_history_rebuild();
    return 0;
}
