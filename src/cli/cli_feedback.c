#include "cli/cli_feedback.h"

#include <stdio.h>

/*
 * Alignment assumptions for future extensions:
 * - CLI headers are the truth source for the text-front-end contract in this file.
 * - This module only formats human-readable feedback and does not mutate game state.
 * - GUI-specific messaging must remain outside the CLI namespace.
 */

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
    printf("Error: %s\n", error_message_for_code(code));
    return 0;
}

/* Print one standardized disabled-feature message for future-not-ready features. */
int cliShowDisabledFeatureMessage(const char *featureName) {
    if (featureName == NULL) {
        printf("This feature is currently unavailable in the CLI build.\n");
        return 1;
    }

    printf("%s is currently disabled in the CLI build.\n", featureName);
    return 0;
}
