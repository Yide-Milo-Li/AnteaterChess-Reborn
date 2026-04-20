#include "cli/cli_feedback.h"

#include <stdio.h>

#include "error/error.h"

/*
 * Alignment assumptions for future extensions:
 * - CLI headers are the truth source for the text-front-end contract in this file.
 * - This module is a renderer, not an error-message authority.
 * - error.h and error_code.h remain the truth source for ErrorCode text selection.
 * - This module only formats human-readable feedback and does not mutate game state.
 * - GUI-specific messaging must remain outside the CLI namespace.
 */

#define ANSI_RESET       "\x1b[0m"
#define ANSI_ERROR       "\x1b[1;31m"
#define ANSI_INFO        "\x1b[1;36m"
#define ANSI_UNAVAILABLE "\x1b[1;33m"

/* Print one CLI-styled error line using the shared error-message source. */
int cliShowErrorMessage(ErrorCode code) {
    printf("%s[Error]%s %s\n", ANSI_ERROR, ANSI_RESET, getErrorMessage(code));
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
