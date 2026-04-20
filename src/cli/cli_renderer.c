#include "cli/cli_renderer.h"

#include <stdio.h>

#include "turn/turn_timer.h"

/*
 * Alignment assumptions for future extensions:
 * - CLI headers are the truth source for the text-front-end contract in this file.
 * - Rendering is read-only and must not embed gameplay decisions or event routing.
 * - The CLI board format is independent from any future GUI presentation model.
 */

/* Convert one board piece into the fixed CLI token used by the ASCII renderer. */
static char cli_piece_token(Piece piece) {
    char token;

    if (piece.type == EMPTY_PIECE || piece.color == EMPTY_COLOR) {
        return '.';
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
            return '.';
    }

    if (piece.color == BLACK) {
        token = (char)(token - 'A' + 'a');
    }

    return token;
}

/* Print the current active-turn label for the CLI gameplay header. */
int cliDisplayTurn(Color turn) {
    switch (turn) {
        case WHITE:
            printf("Turn: White\n");
            return 0;
        case BLACK:
            printf("Turn: Black\n");
            return 0;
        default:
            printf("Turn: Unknown\n");
            return 1;
    }
}

/* Print the shared gameplay status block shown before CLI actions. */
int cliDisplayGameStatus(const GameState *state) {
    int whiteTime;
    int blackTime;

    if (state == NULL) {
        return 1;
    }

    printf("System State: %d\n", (int)state->systemState);
    cliDisplayTurn(state->currentTurn);
    printf("Moves Played: %d\n", state->moveHistory.count);
    printf("Timer: %s\n", state->config.timerEnabled ? "On" : "Off");

    if (state->config.timerEnabled) {
        whiteTime = getRemainingTime(state, WHITE);
        blackTime = getRemainingTime(state, BLACK);
        if (whiteTime >= 0 && blackTime >= 0) {
            printf("White Time: %d\n", whiteTime);
            printf("Black Time: %d\n", blackTime);
        }
    }

    return 0;
}

/* Render the board in a stable ASCII grid with fixed row and column labels. */
int cliRenderBoard(const GameState *state) {
    int row;
    int col;

    if (state == NULL) {
        return 1;
    }

    printf("\n   A B C D E F G H I J\n");
    for (row = 0; row < ROWS; ++row) {
        printf("%d  ", ROWS - row);
        for (col = 0; col < COLS; ++col) {
            printf("%c", cli_piece_token(getPiece(&state->board, createPosition(row, col))));
            if (col < COLS - 1) {
                printf(" ");
            }
        }
        printf("  %d\n", ROWS - row);
    }
    printf("   A B C D E F G H I J\n\n");
    return 0;
}
