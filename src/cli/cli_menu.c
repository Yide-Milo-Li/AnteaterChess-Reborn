#include "cli/cli_menu.h"

#include <stdio.h>

#include "cli/cli_feedback.h"
#include "input/input.h"

/*
 * Alignment assumptions for future extensions:
 * - CLI headers are the truth source for the text-front-end contract in this file.
 * - These helpers own prompt/selection loops, not gameplay rules or FSM transitions.
 * - Game setup consumes the caller-provided GameConfig as seeded context, so
 *   mode selection can happen before setup without widening the public API.
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
            cliShowErrorMessage(ERR_INVALID_MENU_SELECTION);
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

/* Read one supported per-turn timer length for the CLI setup flow. */
static int read_initial_time_seconds(int *seconds) {
    int parsedSeconds;

    if (seconds == NULL) {
        return 1;
    }

    for (;;) {
        printf("Enter per-turn time in seconds (1-3600):\n");
        if (getMenuSelection(&parsedSeconds) != 0 || parsedSeconds < 1 || parsedSeconds > 3600) {
            cliShowErrorMessage(ERR_INVALID_TIMER_SETTING);
            continue;
        }

        *seconds = parsedSeconds;
        return 0;
    }
}

/* Return the stable display label for one public game mode value. */
static const char *game_mode_label(GameMode mode) {
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

/* Return the stable display label for one public color value. */
static const char *color_label(Color color) {
    return (color == BLACK) ? "Black" : "White";
}

/* Return the stable display label for one public AI difficulty. */
static const char *difficulty_label(AIDifficulty difficulty) {
    switch (difficulty) {
        case DIFFICULTY_EASY:
            return "Easy";
        case DIFFICULTY_MEDIUM:
            return "Medium";
        case DIFFICULTY_HARD:
            return "Hard";
        case DIFFICULTY_NONE:
        default:
            return "None";
    }
}

/* Read one human-side selection for human-vs-computer setup. */
static int read_human_side(Color *color) {
    int selection;

    if (color == NULL) {
        return 1;
    }

    printf("Choose the human side:\n");
    printf("1. White\n");
    printf("2. Black\n");
    if (read_selection_in_range(1, 2, &selection) != 0) {
        return 1;
    }

    *color = (selection == 2) ? BLACK : WHITE;
    return 0;
}

/* Read one AI difficulty selection using the public difficulty enum order. */
static int read_ai_difficulty(const char *prompt, AIDifficulty *difficulty) {
    int selection;

    if (difficulty == NULL) {
        return 1;
    }

    printf("%s\n", prompt);
    printf("1. Easy\n");
    printf("2. Medium\n");
    printf("3. Hard\n");
    if (read_selection_in_range(1, 3, &selection) != 0) {
        return 1;
    }

    switch (selection) {
        case 1:
            *difficulty = DIFFICULTY_EASY;
            return 0;
        case 2:
            *difficulty = DIFFICULTY_MEDIUM;
            return 0;
        case 3:
            *difficulty = DIFFICULTY_HARD;
            return 0;
        default:
            return 1;
    }
}

/* Read one bounded AI thinking-time limit for search-based features. */
static int read_ai_time_limit_seconds(int *seconds) {
    int parsedSeconds;

    if (seconds == NULL) {
        return 1;
    }

    for (;;) {
        printf("Enter AI time limit in seconds (1-60):\n");
        if (getMenuSelection(&parsedSeconds) != 0 || parsedSeconds < 1 || parsedSeconds > 60) {
            cliShowErrorMessage(ERR_INVALID_INPUT);
            continue;
        }

        *seconds = parsedSeconds;
        return 0;
    }
}

/* Print one concise setup summary before gameplay starts. */
static void print_setup_summary(GameConfig config) {
    printf("\n[Game Setup]\n");
    printf("Mode: %s\n", game_mode_label(config.mode));

    if (config.mode == MODE_HUMAN_VS_COMPUTER) {
        printf("Human Side: %s\n", color_label(config.playerColor));
    }

    if (config.mode != MODE_HUMAN_VS_HUMAN) {
        printf("AI White: %s\n", difficulty_label(config.aiDifficultyWhite));
        printf("AI Black: %s\n", difficulty_label(config.aiDifficultyBlack));
        printf("AI Time Limit: %ds\n", config.aiTimeLimit);
    }

    printf("Turn Timer: %s\n", config.timerEnabled ? "Enabled" : "Disabled");
    if (config.timerEnabled) {
        printf("Per-Turn Time: %ds\n", config.initialTimeSeconds);
    }
    printf("\n");
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
    printf("2. Human vs Computer\n");
    printf("3. Computer vs Computer\n");
    printf("4. Back\n");
    printf("5. Exit\n");
    return read_selection_in_range(1, 5, selection);
}

/* Collect the CLI setup fields for the caller-selected game mode. */
int cliGetGameSetupConfig(GameConfig *config) {
    GameConfig setupConfig;
    int timerEnabled;

    if (config == NULL) {
        return 1;
    }

    initDefaultGameConfig(&setupConfig);
    setupConfig = *config;

    printf("\n=== Game Setup ===\n");
    printf("Selected mode: %s\n", game_mode_label(setupConfig.mode));

    if (setupConfig.mode == MODE_HUMAN_VS_HUMAN) {
        setupConfig.playerColor = WHITE;
        setupConfig.aiDifficultyWhite = DIFFICULTY_NONE;
        setupConfig.aiDifficultyBlack = DIFFICULTY_NONE;
        setupConfig.aiTimeLimit = 0;
    } else if (setupConfig.mode == MODE_HUMAN_VS_COMPUTER) {
        Color humanColor;
        AIDifficulty aiDifficulty;

        if (read_human_side(&humanColor) != 0
            || read_ai_difficulty("Choose the AI difficulty:", &aiDifficulty) != 0
            || read_ai_time_limit_seconds(&setupConfig.aiTimeLimit) != 0) {
            return 1;
        }

        setupConfig.playerColor = humanColor;
        if (humanColor == WHITE) {
            setupConfig.aiDifficultyWhite = DIFFICULTY_NONE;
            setupConfig.aiDifficultyBlack = aiDifficulty;
        } else {
            setupConfig.aiDifficultyWhite = aiDifficulty;
            setupConfig.aiDifficultyBlack = DIFFICULTY_NONE;
        }
    } else if (setupConfig.mode == MODE_COMPUTER_VS_COMPUTER) {
        if (read_ai_difficulty("Choose White AI difficulty:", &setupConfig.aiDifficultyWhite) != 0
            || read_ai_difficulty("Choose Black AI difficulty:", &setupConfig.aiDifficultyBlack) != 0
            || read_ai_time_limit_seconds(&setupConfig.aiTimeLimit) != 0) {
            return 1;
        }
        setupConfig.playerColor = WHITE;
    } else {
        return 1;
    }

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

    print_setup_summary(setupConfig);
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
