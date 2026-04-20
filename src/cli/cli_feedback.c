#include "cli/cli_feedback.h"

#include <stdio.h>

/*
 * Alignment assumptions for future extensions:
 * - CLI headers are the truth source for the text-front-end contract in this file.
 * - This module only formats human-readable feedback and does not mutate game state.
 * - GUI-specific messaging must remain outside the CLI namespace.
 */

#define ANSI_RESET       "\x1b[0m"
#define ANSI_ERROR       "\x1b[1;31m"
#define ANSI_INFO        "\x1b[1;36m"
#define ANSI_UNAVAILABLE "\x1b[1;33m"

/* Map one public error code to the CLI text shown to the player. */
static const char *error_message_for_code(ErrorCode code) {
    switch (code) {
        case ERR_INVALID_INPUT:
            return "Invalid input.";
        case ERR_EMPTY_SELECTION:
            return "No piece is on that square.";
        case ERR_OPPONENT_PIECE:
            return "That piece belongs to the other player.";
        case ERR_ILLEGAL_MOVE:
            return "Illegal move.";
        case ERR_UNRESOLVED_CHECK:
            return "That move leaves your king in check.";
        case ERR_UNDO_UNAVAILABLE:
            return "Undo is not available.";
        case ERR_HINT_UNAVAILABLE:
            return "Hint is not available in the CLI build.";
        case ERR_NOT_YOUR_TURN:
            return "It is not your turn.";
        case ERR_TIME_UP:
            return "Time is up.";
        case ERR_FATAL:
            return "A fatal error occurred.";
        default:
            return "Unknown error.";
    }
}

/* Print one CLI error message and keep the public API side-effect free otherwise. */
int cliShowErrorMessage(ErrorCode code) {
    printf("%s[Error]%s %s\n", ANSI_ERROR, ANSI_RESET, error_message_for_code(code));
    return 0;
}

/* Print one standardized disabled-feature message for future-not-ready features. */
int cliShowDisabledFeatureMessage(const char *featureName) {
    if (featureName == NULL) {
        printf("%s[Unavailable]%s This feature is currently unavailable in the CLI build.\n",
            ANSI_UNAVAILABLE, ANSI_RESET);
        return 1;
    }

    printf("%s[Unavailable]%s %s is currently disabled in the CLI build.\n",
        ANSI_UNAVAILABLE, ANSI_RESET, featureName);
    return 0;
}
