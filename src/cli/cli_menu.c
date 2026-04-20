#include "cli/cli_menu.h"

#include <stdio.h>

#include "cli/cli_feedback.h"
#include "input/input.h"

/*
 * Alignment assumptions for future extensions:
 * - CLI headers are the truth source for the text-front-end contract in this file.
 * - These helpers own prompt/selection loops, not gameplay rules or FSM transitions.
 * - GUI setup flows should remain separate and must not reuse CLI prompt wording.
 */

/* Read one bounded integer selection and keep reprompting until it is valid. */
static int read_selection_in_range(int minValue, int maxValue, int *selection) {
    int parsedSelection;

    if (selection == NULL) {
        return 1;
    }

    for (;;) {
        if (getMenuSelection(&parsedSelection) != 0) {
            cliShowErrorMessage(ERR_INVALID_INPUT);
            continue;
        }

        if (parsedSelection < minValue || parsedSelection > maxValue) {
            cliShowErrorMessage(ERR_INVALID_INPUT);
            continue;
        }

        *selection = parsedSelection;
        return 0;
    }
}

/* Prompt for a yes-or-no toggle and normalize it into 0 or 1. */
static int read_yes_no(const char *prompt, int *value) {
    int selection;

    if (value == NULL) {
        return 1;
    }

    printf("%s\n", prompt);
    printf("1. Yes\n");
    printf("2. No\n");
    if (read_selection_in_range(1, 2, &selection) != 0) {
        return 1;
    }

    *value = (selection == 1) ? 1 : 0;
    return 0;
}

/* Read a non-negative turn length for the CLI setup flow. */
static int read_initial_time_seconds(int *seconds) {
    int parsedSeconds;

    if (seconds == NULL) {
        return 1;
    }

    for (;;) {
        printf("Enter per-turn time in seconds (1-3600):\n");
        if (getMenuSelection(&parsedSeconds) != 0 || parsedSeconds < 1 || parsedSeconds > 3600) {
            cliShowErrorMessage(ERR_INVALID_INPUT);
            continue;
        }

        *seconds = parsedSeconds;
        return 0;
    }
}

/* Show the main menu and return the validated selection. */
int cliGetMainMenuSelection(int *selection) {
    printf("\n=== Anteater Chess CLI ===\n");
    printf("1. New Game\n");
    printf("2. Exit\n");
    return read_selection_in_range(1, 2, selection);
}

/* Show the game-mode menu and return the validated selection. */
int cliGetGameModeSelection(int *selection) {
    printf("\n=== Game Mode ===\n");
    printf("1. Human vs Human\n");
    printf("2. Human vs Computer (Disabled)\n");
    printf("3. Computer vs Computer (Disabled)\n");
    printf("4. Back\n");
    printf("5. Exit\n");
    return read_selection_in_range(1, 5, selection);
}

/* Collect the CLI setup fields needed to start one human-vs-human game. */
int cliGetGameSetupConfig(GameConfig *config) {
    GameConfig setupConfig;
    int timerEnabled;

    if (config == NULL) {
        return 1;
    }

    initDefaultGameConfig(&setupConfig);
    setupConfig.mode = MODE_HUMAN_VS_HUMAN;
    setupConfig.aiDifficultyWhite = DIFFICULTY_NONE;
    setupConfig.aiDifficultyBlack = DIFFICULTY_NONE;
    setupConfig.aiTimeLimit = 0;

    if (read_yes_no("Enable turn timer?", &timerEnabled) != 0) {
        return 1;
    }

    setupConfig.timerEnabled = timerEnabled;
    if (timerEnabled) {
        if (read_initial_time_seconds(&setupConfig.initialTimeSeconds) != 0) {
            return 1;
        }
    } else {
        setupConfig.initialTimeSeconds = 0;
    }

    *config = setupConfig;
    return 0;
}

/* Show the end-game menu and return the validated next-step selection. */
int cliShowEndGameMenu(const GameState *state, int *selection) {
    if (state == NULL || selection == NULL) {
        return 1;
    }

    printf("\n=== Game Over ===\n");
    printf("Result: %d\n", (int)state->result);
    printf("1. New Game\n");
    printf("2. Main Menu\n");
    printf("3. Exit\n");
    return read_selection_in_range(1, 3, selection);
}
