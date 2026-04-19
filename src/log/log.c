#include "log/log.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define MAKE_DIRECTORY(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MAKE_DIRECTORY(path) mkdir((path), 0777)
#endif

#include "time/clock.h"

/*
 * Alignment assumptions for future extensions:
 * - The public log API is the only contract other modules should depend on.
 * - Session log metadata stays local to this module so GameState and Move remain unchanged.
 * - Undo never writes a standalone log entry; rebuild rewrites the file from surviving history.
 */

static FILE *logFile = NULL;
static char sessionLogPath[256];
static int moveTimestampsSeconds[MAX_MOVES];

/* Return the human-readable name for one piece type. */
static const char *piece_type_to_string(PieceType type) {
    switch (type) {
        case ANT:
            return "Ant";
        case ROOK:
            return "Rook";
        case KNIGHT:
            return "Knight";
        case BISHOP:
            return "Bishop";
        case QUEEN:
            return "Queen";
        case KING:
            return "King";
        case ANTEATER:
            return "Anteater";
        case EMPTY_PIECE:
        default:
            return "Empty";
    }
}

/* Return the human-readable label for one color. */
static const char *color_to_string(Color color) {
    switch (color) {
        case WHITE:
            return "White";
        case BLACK:
            return "Black";
        case EMPTY_COLOR:
        default:
            return "None";
    }
}

/* Return the human-readable label for one configured game mode. */
static const char *game_mode_to_string(GameMode mode) {
    switch (mode) {
        case MODE_HUMAN_VS_HUMAN:
            return "Human vs Human";
        case MODE_HUMAN_VS_COMPUTER:
            return "Human vs Computer";
        case MODE_COMPUTER_VS_COMPUTER:
            return "Computer vs Computer";
        default:
            return "Unknown";
    }
}

/* Return the human-readable label for one AI difficulty value. */
static const char *difficulty_to_string(AIDifficulty difficulty) {
    switch (difficulty) {
        case DIFFICULTY_NONE:
            return "None";
        case DIFFICULTY_EASY:
            return "Easy";
        case DIFFICULTY_MEDIUM:
            return "Medium";
        case DIFFICULTY_HARD:
            return "Hard";
        default:
            return "Unknown";
    }
}

/* Return the human-readable label for one terminal result. */
static const char *result_to_string(GameResult result) {
    switch (result) {
        case RESULT_NONE:
            return "In Progress";
        case RESULT_WHITE_WIN:
            return "White Win";
        case RESULT_BLACK_WIN:
            return "Black Win";
        case RESULT_DRAW:
            return "Draw";
        case RESULT_TERMINATED_BY_USER:
            return "Terminated By User";
        default:
            return "Unknown";
    }
}

/* Return the human-readable label for one special move marker. */
static const char *special_move_to_string(SpecialMove type) {
    switch (type) {
        case NO_SPECIAL_MOVE:
            return "None";
        case CASTLING_KINGSIDE:
            return "Castling Kingside";
        case CASTLING_QUEENSIDE:
            return "Castling Queenside";
        case EN_PASSANT:
            return "En Passant";
        case PROMOTION_QUEEN:
            return "Promotion to Queen";
        case PROMOTION_ROOK:
            return "Promotion to Rook";
        case PROMOTION_BISHOP:
            return "Promotion to Bishop";
        case PROMOTION_KNIGHT:
            return "Promotion to Knight";
        case ANTEATER_CAPTURE:
            return "Anteater Capture";
        default:
            return "Unknown";
    }
}

/* Format one board position into standard file-rank notation. */
static void format_position(Position pos, char buffer[4]) {
    if (!isValidPosition(pos)) {
        buffer[0] = '?';
        buffer[1] = '?';
        buffer[2] = '\0';
        return;
    }

    buffer[0] = (char) ('A' + pos.col);
    buffer[1] = (char) ('0' + (ROWS - pos.row));
    buffer[2] = '\0';
}

/* Format elapsed seconds into the HH:MM:SS layout required by the log spec. */
static void format_elapsed_time(int elapsedSeconds, char buffer[16]) {
    int hours;
    int minutes;
    int seconds;

    if (elapsedSeconds < 0) {
        elapsedSeconds = 0;
    }

    hours = elapsedSeconds / 3600;
    minutes = (elapsedSeconds % 3600) / 60;
    seconds = elapsedSeconds % 60;
    snprintf(buffer, 16, "%02d:%02d:%02d", hours, minutes, seconds);
}

/* Build the player label, explicitly tagging AI-controlled sides. */
static void format_player_label(const GameState *state, Color color, char buffer[32]) {
    const char *baseLabel;

    baseLabel = color_to_string(color);
    if (state != NULL && (color == WHITE || color == BLACK) && state->players[color].type == AI) {
        snprintf(buffer, 32, "%s (AI)", baseLabel);
        return;
    }

    snprintf(buffer, 32, "%s", baseLabel);
}

/* Create the logs directory if it does not already exist. */
static int ensure_logs_directory(void) {
    if (MAKE_DIRECTORY("logs") == 0 || errno == EEXIST) {
        return 0;
    }

    return 1;
}

/* Build the timestamped session log path required by the architecture spec. */
static int build_log_path(char buffer[256]) {
    time_t now;
    struct tm *localNow;

    now = time(NULL);
    if (now == (time_t) -1) {
        return 1;
    }

    localNow = localtime(&now);
    if (localNow == NULL) {
        return 1;
    }

    snprintf(
        buffer,
        256,
        "logs/game_%04d%02d%02d_%02d%02d%02d.log",
        localNow->tm_year + 1900,
        localNow->tm_mon + 1,
        localNow->tm_mday,
        localNow->tm_hour,
        localNow->tm_min,
        localNow->tm_sec
    );
    return 0;
}

/* Reopen the current session file in truncate mode for a full rebuild. */
static int reopen_session_log_for_rebuild(void) {
    if (sessionLogPath[0] == '\0') {
        return 1;
    }

    if (logFile != NULL) {
        fclose(logFile);
        logFile = NULL;
    }

    logFile = fopen(sessionLogPath, "w");
    if (logFile == NULL) {
        return 1;
    }

    return 0;
}

/* Write the configuration header section for one game session. */
static int write_header_section(const GameConfig *config) {
    if (logFile == NULL || config == NULL) {
        return 1;
    }

    fprintf(logFile, "Anteater Chess Game Log\n");
    fprintf(logFile, "Mode: %s\n", game_mode_to_string(config->mode));
    fprintf(logFile, "Player Color: %s\n", color_to_string(config->playerColor));
    fprintf(logFile, "AI White: %s\n", difficulty_to_string(config->aiDifficultyWhite));
    fprintf(logFile, "AI Black: %s\n", difficulty_to_string(config->aiDifficultyBlack));
    fprintf(logFile, "Timer Enabled: %s\n", config->timerEnabled ? "Yes" : "No");
    fprintf(logFile, "Initial Time Per Turn: %ds\n", config->initialTimeSeconds);
    fprintf(logFile, "AI Time Limit: %ds\n", config->aiTimeLimit);
    fprintf(logFile, "\nMove History:\n");
    return 0;
}

/* Write one move line in the strict replay-friendly format. */
static int write_move_line(const GameState *state, int moveNumber, int elapsedSeconds, Move move) {
    char timestamp[16];
    char playerLabel[32];
    char from[4];
    char to[4];

    if (logFile == NULL) {
        return 1;
    }

    format_elapsed_time(elapsedSeconds, timestamp);
    format_player_label(state, move.movedPiece.color, playerLabel);
    format_position(move.from, from);
    format_position(move.to, to);

    fprintf(
        logFile,
        "[Move %03d] %s | %s | %s %s -> %s",
        moveNumber,
        timestamp,
        playerLabel,
        piece_type_to_string(move.movedPiece.type),
        from,
        to
    );

    if (move.captureCount > 0) {
        fprintf(logFile, " | Captures: %d", move.captureCount);
    }

    if (move.specialType != NO_SPECIAL_MOVE) {
        fprintf(logFile, " | Special: %s", special_move_to_string(move.specialType));
    }

    fprintf(logFile, "\n");
    return 0;
}

/* Append the termination summary for the current session. */
static int write_termination_section(const GameState *state) {
    char elapsedBuffer[16];

    if (logFile == NULL || state == NULL) {
        return 1;
    }

    format_elapsed_time(getElapsedTimeSeconds(), elapsedBuffer);
    fprintf(logFile, "\nTermination Summary:\n");
    fprintf(logFile, "Result: %s\n", result_to_string(state->result));
    fprintf(logFile, "Total Elapsed Time: %s\n", elapsedBuffer);
    return 0;
}

/* Initialize the persistent session log and choose its output path. */
int initLog(const GameConfig *config) {
    (void) config;

    closeLog();
    if (ensure_logs_directory() != 0) {
        return 1;
    }

    if (build_log_path(sessionLogPath) != 0) {
        sessionLogPath[0] = '\0';
        return 1;
    }

    logFile = fopen(sessionLogPath, "w");
    if (logFile == NULL) {
        sessionLogPath[0] = '\0';
        return 1;
    }

    memset(moveTimestampsSeconds, 0, sizeof(moveTimestampsSeconds));
    return 0;
}

/* Write the header section that describes the configured game session. */
int logGameStart(const GameConfig *config) {
    if (write_header_section(config) != 0) {
        return 1;
    }

    fflush(logFile);
    return 0;
}

/* Append one validated move and remember its original elapsed timestamp. */
int logMove(const GameState *state, Move move) {
    int moveIndex;

    if (logFile == NULL || state == NULL) {
        return 1;
    }

    moveIndex = state->moveHistory.count - 1;
    if (moveIndex < 0 || moveIndex >= MAX_MOVES) {
        return 1;
    }

    moveTimestampsSeconds[moveIndex] = getElapsedTimeSeconds();

    if (write_move_line(state, moveIndex + 1, moveTimestampsSeconds[moveIndex], move) != 0) {
        return 1;
    }

    fflush(logFile);
    return 0;
}

/* Rewrite the session log so it matches the current surviving move history. */
int rebuildLogFromHistory(const GameState *state) {
    int index;

    if (state == NULL || reopen_session_log_for_rebuild() != 0) {
        return 1;
    }

    if (write_header_section(&state->config) != 0) {
        return 1;
    }

    for (index = 0; index < state->moveHistory.count; ++index) {
        if (write_move_line(state, index + 1, moveTimestampsSeconds[index], state->moveHistory.moves[index]) != 0) {
            return 1;
        }
    }

    if (state->gameOver) {
        if (write_termination_section(state) != 0) {
            return 1;
        }
    }

    fflush(logFile);
    return 0;
}

/* Append the termination summary when the game reaches a terminal state. */
int logGameEnd(const GameState *state) {
    if (write_termination_section(state) != 0) {
        return 1;
    }

    fflush(logFile);
    return 0;
}

/* Close the current session file and clear module-local session state. */
void closeLog(void) {
    if (logFile != NULL) {
        fclose(logFile);
        logFile = NULL;
    }

    sessionLogPath[0] = '\0';
    memset(moveTimestampsSeconds, 0, sizeof(moveTimestampsSeconds));
}
