#include <stdio.h>

#include "log/log.h"

/* Keep a single writable handle for the session so every log API appends to
 * the same human-readable transcript. */
static FILE *logFile = NULL;

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

/* The move lines intentionally stay structured so they are readable in a text
 * editor and easy to rebuild from move history later. */
static int write_move_line(Move move) {
    char from[4];
    char to[4];

    if (logFile == NULL) {
        return 0;
    }

    format_position(move.from, from);
    format_position(move.to, to);

    fprintf(
        logFile,
        "Move: %s %s %s -> %s | Captures: %d | Special: %s\n",
        color_to_string(move.movedPiece.color),
        piece_type_to_string(move.movedPiece.type),
        from,
        to,
        move.captureCount,
        special_move_to_string(move.specialType)
    );
    return 1;
}

int initLog(const GameConfig *config) {
    (void) config;

    closeLog();
    logFile = fopen("game.log", "w");
    if (logFile == NULL) {
        return 0;
    }

    return 1;
}

int logGameStart(const GameConfig *config) {
    if (logFile == NULL || config == NULL) {
        return 0;
    }

    fprintf(logFile, "Anteater Chess Game Started\n");
    fprintf(logFile, "Mode: %s\n", game_mode_to_string(config->mode));
    fprintf(logFile, "Player Color: %s\n", color_to_string(config->playerColor));
    fprintf(logFile, "AI White: %s\n", difficulty_to_string(config->aiDifficultyWhite));
    fprintf(logFile, "AI Black: %s\n", difficulty_to_string(config->aiDifficultyBlack));
    fprintf(logFile, "Timer Enabled: %s\n", config->timerEnabled ? "Yes" : "No");
    fprintf(logFile, "Initial Time: %d\n", config->initialTimeSeconds);
    fprintf(logFile, "---------------------------\n");
    fflush(logFile);
    return 1;
}

int logMove(const GameState *state, Move move) {
    (void) state;

    if (!write_move_line(move)) {
        return 0;
    }

    fflush(logFile);
    return 1;
}

int rebuildLogFromHistory(const GameState *state) {
    int index;

    /* Rebuild walks the stored history in order so a restored session produces
     * the same readable move transcript as live logging. */
    if (logFile == NULL || state == NULL) {
        return 0;
    }

    fprintf(logFile, "Rebuilding Log From History\n");
    for (index = 0; index < state->moveHistory.count; ++index) {
        write_move_line(state->moveHistory.moves[index]);
    }
    fflush(logFile);
    return 1;
}

int logGameEnd(const GameState *state) {
    if (logFile == NULL || state == NULL) {
        return 0;
    }

    fprintf(logFile, "Game Over: %s\n", result_to_string(state->result));
    fflush(logFile);
    return 1;
}

void closeLog(void) {
    if (logFile != NULL) {
        fclose(logFile);
        logFile = NULL;
    }
}
