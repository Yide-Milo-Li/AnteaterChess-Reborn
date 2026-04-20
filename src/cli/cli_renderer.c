#include "cli/cli_renderer.h"

#include <stdint.h>
#include <stdio.h>

#include "time/clock.h"
#include "turn/turn_timer.h"

/*
 * Alignment assumptions for future extensions:
 * - CLI headers are the truth source for the text-front-end contract in this file.
 * - Rendering is read-only and must not embed gameplay decisions or event routing.
 * - The CLI board format is independent from any future GUI presentation model.
 */

/* ANSI style helpers use broadly supported standard color sequences. */
#define ANSI_RESET          "\x1b[0m"
#define ANSI_BOLD           "\x1b[1m"
#define ANSI_ACCENT         "\x1b[1;36m"
#define ANSI_BORDER         "\x1b[36m"
#define ANSI_WHITE_PIECE    "\x1b[1;96m"
#define ANSI_BLACK_PIECE    "\x1b[1;33m"
#define ANSI_LIGHT_SQUARE   "\x1b[47m"
#define ANSI_DARK_SQUARE    "\x1b[100m"
#define ANSI_WARNING        "\x1b[1;31m"
#define ANSI_WHITE_BADGE    "\x1b[1;30;46m"
#define ANSI_BLACK_BADGE    "\x1b[1;30;43m"

/* ASCII box helpers avoid Unicode encoding issues across terminals and logs. */
#define BOX_H               "---"
#define BOX_V               "|"
#define BOX_TL              "+"
#define BOX_TR              "+"
#define BOX_BL              "+"
#define BOX_BR              "+"
#define BOX_LTEE            "+"
#define BOX_RTEE            "+"
#define BOX_TTEE            "+"
#define BOX_BTEE            "+"
#define BOX_CROSS           "+"

/* Keep row labels fixed-width so the left and right borders stay aligned. */
#define ROW_LABEL_FORMAT    "%2d"

/* Convert one board piece into the fixed CLI token used by the board panel. */
static char cli_piece_token(Piece piece) {
    char token;

    if (piece.type == EMPTY_PIECE || piece.color == EMPTY_COLOR) {
        return ' ';
    }

    switch (piece.type) {
        case ANT:
            token = 'A';
            break;
        case ANTEATER:
            token = 'E';
            break;
        case ROOK:
            token = 'R';
            break;
        case KNIGHT:
            token = 'N';
            break;
        case BISHOP:
            token = 'B';
            break;
        case QUEEN:
            token = 'Q';
            break;
        case KING:
            token = 'K';
            break;
        case EMPTY_PIECE:
        default:
            return ' ';
    }

    if (piece.color == BLACK) {
        token = (char)(token - 'A' + 'a');
    }

    return token;
}

/* Return the stable status label for one public system state value. */
static const char *system_state_label(SystemState state) {
    switch (state) {
        case INIT_STATE:
            return "Init";
        case MAIN_MENU_STATE:
            return "Main Menu";
        case GAME_MODE_SELECTION_STATE:
            return "Mode Select";
        case GAME_SETUP_STATE:
            return "Game Setup";
        case GAMEPLAY_STATE:
            return "Gameplay";
        case END_GAME_MENU_STATE:
            return "Endgame Menu";
        case GAME_TERMINATION_STATE:
            return "Terminating";
        case EXIT_STATE:
            return "Exit";
        default:
            return "Unknown";
    }
}

/* Return the stable status label for one public game result value. */
static const char *game_result_label(GameResult result) {
    switch (result) {
        case RESULT_NONE:
            return "In Progress";
        case RESULT_WHITE_WIN:
            return "White Wins";
        case RESULT_BLACK_WIN:
            return "Black Wins";
        case RESULT_DRAW:
            return "Draw";
        case RESULT_TERMINATED_BY_USER:
            return "Ended by User";
        default:
            return "Unknown";
    }
}

/* Format elapsed seconds into the shared HH:MM:SS display layout. */
static void format_time_value(int64_t totalSeconds, char buffer[32]) {
    int64_t hours;
    int64_t minutes;
    int64_t seconds;

    if (totalSeconds < 0) {
        totalSeconds = 0;
    }

    hours = totalSeconds / 3600;
    minutes = (totalSeconds % 3600) / 60;
    seconds = totalSeconds % 60;
    snprintf(buffer, 32, "%02lld:%02lld:%02lld",
        (long long)hours,
        (long long)minutes,
        (long long)seconds);
}

/* Return the badge style used to highlight the side to move. */
static const char *turn_badge_style(Color turn) {
    return (turn == BLACK) ? ANSI_BLACK_BADGE : ANSI_WHITE_BADGE;
}

/* Return the square background used for one checkerboard coordinate. */
static const char *square_background_style(int row, int col) {
    return (((row + col) % 2) == 0) ? ANSI_LIGHT_SQUARE : ANSI_DARK_SQUARE;
}

/* Return the piece foreground style for one side. */
static const char *piece_foreground_style(Piece piece) {
    return (piece.color == BLACK) ? ANSI_BLACK_PIECE : ANSI_WHITE_PIECE;
}

/* Print one status row inside the game-status panel with optional row emphasis. */
static void print_status_row(const char *rowStyle, const char *label, const char *value) {
    const char *effectiveStyle = (rowStyle != NULL) ? rowStyle : "";

    printf("%s| %-13s %-26s |%s\n", effectiveStyle, label, value, ANSI_RESET);
}

/* Print one fully styled board cell, including square background and piece color. */
static void print_board_cell(int row, int col, Piece piece) {
    const char *backgroundStyle = square_background_style(row, col);
    char token = cli_piece_token(piece);

    if (token == ' ') {
        printf("|%s   %s", backgroundStyle, ANSI_RESET);
        return;
    }

    printf("|%s%s %c %s", backgroundStyle, piece_foreground_style(piece), token, ANSI_RESET);
}

/* Print the current active-turn badge for the CLI gameplay header. */
int cliDisplayTurn(Color turn) {
    switch (turn) {
        case WHITE:
            printf("%s%s  WHITE TO MOVE  %s\n", ANSI_BOLD, turn_badge_style(turn), ANSI_RESET);
            return 0;
        case BLACK:
            printf("%s%s  BLACK TO MOVE  %s\n", ANSI_BOLD, turn_badge_style(turn), ANSI_RESET);
            return 0;
        default:
            printf("%s[Info]%s Unknown side to move.\n", ANSI_ACCENT, ANSI_RESET);
            return 1;
    }
}

/* Print the structured gameplay status panel shown above the rendered board. */
int cliDisplayGameStatus(const GameState *state) {
    char moveCountText[16];
    char elapsedText[32];
    char whiteTimerText[32];
    char blackTimerText[32];
    char timerModeText[16];
    int whiteTime;
    int blackTime;
    const char *turnValue;

    if (state == NULL) {
        return 1;
    }

    snprintf(moveCountText, sizeof(moveCountText), "%d", state->moveHistory.count);
    snprintf(timerModeText, sizeof(timerModeText), "%s", state->config.timerEnabled ? "Enabled" : "Disabled");
    format_time_value(getElapsedTimeSeconds(), elapsedText);

    whiteTime = getRemainingTime(state, WHITE);
    blackTime = getRemainingTime(state, BLACK);
    if (whiteTime >= 0) {
        format_time_value(whiteTime, whiteTimerText);
    } else {
        snprintf(whiteTimerText, sizeof(whiteTimerText), "--:--:--");
    }

    if (blackTime >= 0) {
        format_time_value(blackTime, blackTimerText);
    } else {
        snprintf(blackTimerText, sizeof(blackTimerText), "--:--:--");
    }

    turnValue = (state->currentTurn == BLACK) ? "Black" : "White";

    printf("%s+------------- Game Status ----------------------+%s\n", ANSI_BORDER, ANSI_RESET);
    print_status_row(NULL, "State", system_state_label(state->systemState));
    print_status_row(ANSI_ACCENT, "Current Turn", turnValue);
    print_status_row(NULL, "Moves Played", moveCountText);
    print_status_row(NULL, "Elapsed Time", elapsedText);
    print_status_row(NULL, "Turn Timer", timerModeText);

    if (state->config.timerEnabled) {
        print_status_row(
            (state->currentTurn == WHITE && whiteTime >= 0 && whiteTime <= 10) ? ANSI_WARNING
                : (state->currentTurn == WHITE ? ANSI_ACCENT : NULL),
            "White Timer",
            whiteTimerText
        );
        print_status_row(
            (state->currentTurn == BLACK && blackTime >= 0 && blackTime <= 10) ? ANSI_WARNING
                : (state->currentTurn == BLACK ? ANSI_ACCENT : NULL),
            "Black Timer",
            blackTimerText
        );
    } else {
        print_status_row(NULL, "White Timer", "--:--:--");
        print_status_row(NULL, "Black Timer", "--:--:--");
    }

    if (state->gameOver) {
        print_status_row(ANSI_WARNING, "Result", game_result_label(state->result));
    } else {
        print_status_row(NULL, "Result", game_result_label(RESULT_NONE));
    }

    printf("%s+-----------------------------------------------+%s\n", ANSI_BORDER, ANSI_RESET);
    return 0;
}

/* Render the board as an ASCII panel with ANSI-styled squares and pieces. */
int cliRenderBoard(const GameState *state) {
    static const char *columnLabels[COLS] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J"};
    int col;
    int row;

    if (state == NULL) {
        return 1;
    }

    printf("%s      Board%s\n", ANSI_ACCENT, ANSI_RESET);
    printf("%s      ", ANSI_BORDER);
    for (col = 0; col < COLS; ++col) {
        printf(" %s ", columnLabels[col]);
        if (col < COLS - 1) {
            printf(" ");
        }
    }
    printf("%s\n", ANSI_RESET);

    printf("%s    %s", ANSI_BORDER, BOX_TL);
    for (col = 0; col < COLS; ++col) {
        printf(BOX_H);
        if (col < COLS - 1) {
            printf(BOX_TTEE);
        }
    }
    printf("%s%s\n", BOX_TR, ANSI_RESET);

    for (row = 0; row < ROWS; ++row) {
        printf("%s " ROW_LABEL_FORMAT " %s", ANSI_BORDER, ROWS - row, ANSI_RESET);
        for (col = 0; col < COLS; ++col) {
            print_board_cell(row, col, getPiece(&state->board, createPosition(row, col)));
        }
        printf("%s%s " ROW_LABEL_FORMAT "%s\n", ANSI_BORDER, BOX_V, ROWS - row, ANSI_RESET);

        if (row < ROWS - 1) {
            printf("%s    %s", ANSI_BORDER, BOX_LTEE);
            for (col = 0; col < COLS; ++col) {
                printf(BOX_H);
                if (col < COLS - 1) {
                    printf(BOX_CROSS);
                }
            }
            printf("%s%s\n", BOX_RTEE, ANSI_RESET);
        }
    }

    printf("%s    %s", ANSI_BORDER, BOX_BL);
    for (col = 0; col < COLS; ++col) {
        printf(BOX_H);
        if (col < COLS - 1) {
            printf(BOX_BTEE);
        }
    }
    printf("%s%s\n", BOX_BR, ANSI_RESET);

    printf("%s      ", ANSI_BORDER);
    for (col = 0; col < COLS; ++col) {
        printf(" %s ", columnLabels[col]);
        if (col < COLS - 1) {
            printf(" ");
        }
    }
    printf("%s\n", ANSI_RESET);

    printf("%sLegend:%s %sWhite%s = uppercase cyan, %sBlack%s = lowercase gold.\n\n",
        ANSI_ACCENT,
        ANSI_RESET,
        ANSI_WHITE_PIECE,
        ANSI_RESET,
        ANSI_BLACK_PIECE,
        ANSI_RESET
    );
    return 0;
}
