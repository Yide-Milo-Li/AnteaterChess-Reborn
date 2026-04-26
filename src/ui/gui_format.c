#include "gui_internal.h"

#include <stdio.h>

int gui_current_turn_is_ai(const GameState *state) {
    if (state == NULL || state->currentTurn < WHITE || state->currentTurn > BLACK) {
        return 0;
    }

    return state->players[state->currentTurn].type == AI;
}

const char *gui_game_mode_title(GameMode mode) {
    switch (mode) {
        case MODE_HUMAN_VS_HUMAN:
            return "Human vs. Human";
        case MODE_HUMAN_VS_COMPUTER:
            return "Human vs. Computer";
        case MODE_COMPUTER_VS_COMPUTER:
            return "Computer vs. Computer";
        default:
            return "Game Setup";
    }
}

const char *gui_game_result_text(GameResult result) {
    switch (result) {
        case RESULT_WHITE_WIN:
            return "White wins.";
        case RESULT_BLACK_WIN:
            return "Black wins.";
        case RESULT_DRAW:
            return "Draw.";
        case RESULT_TERMINATED_BY_USER:
            return "Game ended by user.";
        case RESULT_NONE:
        default:
            return "Game over.";
    }
}

AIDifficulty gui_difficulty_from_index(int index) {
    switch (index) {
        case 0:
            return DIFFICULTY_EASY;
        case 1:
            return DIFFICULTY_MEDIUM;
        case 2:
            return DIFFICULTY_HARD;
        default:
            return DIFFICULTY_EASY;
    }
}

int gui_difficulty_index(AIDifficulty difficulty) {
    switch (difficulty) {
        case DIFFICULTY_MEDIUM:
            return 1;
        case DIFFICULTY_HARD:
            return 2;
        case DIFFICULTY_EASY:
        case DIFFICULTY_NONE:
        default:
            return 0;
    }
}

void gui_format_elapsed_text(char buffer[32], int64_t elapsedSeconds) {
    int hours;
    int minutes;
    int seconds;

    if (buffer == NULL) {
        return;
    }

    if (elapsedSeconds < 0) {
        elapsedSeconds = 0;
    }

    hours = (int) (elapsedSeconds / 3600);
    minutes = (int) ((elapsedSeconds % 3600) / 60);
    seconds = (int) (elapsedSeconds % 60);
    snprintf(buffer, 32, "%02d:%02d:%02d", hours, minutes, seconds);
}

void gui_format_timer_text(char buffer[32], const char *prefix, int seconds) {
    int hours;
    int minutes;
    int remainderSeconds;

    if (buffer == NULL || prefix == NULL) {
        return;
    }

    if (seconds < 0) {
        snprintf(buffer, 32, "%s --:--:--", prefix);
        return;
    }

    hours = seconds / 3600;
    minutes = (seconds % 3600) / 60;
    remainderSeconds = seconds % 60;
    snprintf(buffer, 32, "%s %02d:%02d:%02d", prefix, hours, minutes, remainderSeconds);
}

void gui_format_position_text(Position pos, char buffer[8]) {
    if (buffer == NULL) {
        return;
    }

    if (!isValidPosition(pos)) {
        snprintf(buffer, 8, "??");
        return;
    }

    buffer[0] = (char) ('A' + pos.col);
    buffer[1] = (char) ('0' + (8 - pos.row));
    buffer[2] = '\0';
}

void gui_format_hint_text(Move move, char buffer[64]) {
    char fromText[8];
    char toText[8];

    if (buffer == NULL) {
        return;
    }

    gui_format_position_text(move.from, fromText);
    gui_format_position_text(move.to, toText);
    snprintf(buffer, 64, "Hint: %s -> %s", fromText, toText);
}

const char *gui_get_piece_icon(Piece piece) {
    static const char *whiteIcons[] = {
        "assets/WhiteAntsvg.svg",
        "assets/WhiteRook.svg",
        "assets/WhiteKnight.svg",
        "assets/WhiteBishop.svg",
        "assets/WhiteQueen.svg",
        "assets/WhiteKing.svg",
        "assets/WhiteAnteater.svg"
    };
    static const char *blackIcons[] = {
        "assets/BlackAnt.svg",
        "assets/BlackRook.svg",
        "assets/BlackKnight.svg",
        "assets/BlackBishop.svg",
        "assets/BlackQueen.svg",
        "assets/BlackKing.svg",
        "assets/BlackAnteater.svg"
    };
    int index;

    if (!isValidPiece(piece) || piece.type == EMPTY_PIECE) {
        return NULL;
    }

    index = piece.type - ANT;
    if (index < 0 || index >= 7) {
        return NULL;
    }

    return piece.color == WHITE ? whiteIcons[index] : blackIcons[index];
}
