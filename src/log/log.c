#include <stdio.h>
#include "log/log.h"

static FILE *logFile = NULL;

/* Open the log file for the session */
int initLog(const GameConfig *config) {
    logFile = fopen("game.log", "w");
    if (logFile == NULL) return 0;
    return 1;
}

/* Log the basic game information at the start */
int logGameStart(const GameConfig *config) {
    if (logFile == NULL) return 0;
    fprintf(logFile, "Anteater Chess Game Started\n");
    fprintf(logFile, "---------------------------\n");
    // You can add more info from config here
    fflush(logFile);
    return 1;
}

/* Log every move made by players */
int logMove(const GameState *state, Move move) {
    if (logFile == NULL) return 0;
    // Format: Move: (row,col) to (row,col)
    fprintf(logFile, "Move: (%d,%d) to (%d,%d)\n", 
            move.from.row, move.from.col, 
            move.to.row, move.to.col);
    fflush(logFile);
    return 1;
}

/* Optional: Re-create log if loading a saved game */
int rebuildLogFromHistory(const GameState *state) {
    if (logFile == NULL) return 0;
    fprintf(logFile, "Log rebuilt from history.\n");
    return 1;
}

/* Log the winner and final status */
int logGameEnd(const GameState *state) {
    if (logFile == NULL) return 0;
    fprintf(logFile, "Game Over.\n");
    fflush(logFile);
    return 1;
}

/* Safely close the file handle */
void closeLog(void) {
    if (logFile != NULL) {
        fclose(logFile);
        logFile = NULL;
    }
}