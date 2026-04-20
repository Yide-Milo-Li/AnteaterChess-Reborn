#include "cli/cli_gameplay.h"

#include <stdio.h>

#include "cli/cli_feedback.h"
#include "input/input.h"

/*
 * Alignment assumptions for future extensions:
 * - CLI headers are the truth source for the text-front-end contract in this file.
 * - Gameplay prompts emit parsed commands or menu selections, not events or board mutations.
 * - GUI move widgets should not inherit CLI prompt flow or wording.
 */

#define ANSI_RESET  "\x1b[0m"
#define ANSI_ACCENT "\x1b[1;36m"

/* Read one validated gameplay action selection from the CLI action menu. */
int cliGetGameplayAction(int *selection) {
    if (selection == NULL) {
        return 1;
    }

    printf("%s+---------------- Actions ----------------+%s\n", ANSI_ACCENT, ANSI_RESET);
    printf("| 1. Make move                           |\n");
    printf("| 2. Undo                                |\n");
    printf("| 3. Leave game                          |\n");
    printf("| 4. Exit program                        |\n");
    printf("| 5. Show move format hint               |\n");
    printf("%s+-----------------------------------------+%s\n", ANSI_ACCENT, ANSI_RESET);
    printf("%sSelect an action:%s\n", ANSI_ACCENT, ANSI_RESET);

    for (;;) {
        if (getMenuSelection(selection) != 0 || *selection < 1 || *selection > 5) {
            cliShowErrorMessage(ERR_INVALID_INPUT);
            continue;
        }

        return 0;
    }
}

/* Read one CLI move command using the shared input parser contract. */
int cliGetMoveCommand(Command *cmd) {
    if (cmd == NULL) {
        return 1;
    }

    printf("%s[Move Input]%s Enter move as: FROM TO (example: E2 E4)\n", ANSI_ACCENT, ANSI_RESET);
    return getMoveInput(cmd);
}

/* Print one stable move-format hint for the CLI gameplay prompt. */
int cliShowMoveFormatHint(void) {
    printf("%s+---------------- Move Format ----------------+%s\n", ANSI_ACCENT, ANSI_RESET);
    printf("| Move format: FROM TO                       |\n");
    printf("| Example: E2 E4                             |\n");
    printf("| Coordinates ignore case and                |\n");
    printf("| tolerate leading/trailing spaces.          |\n");
    printf("%s+---------------------------------------------+%s\n", ANSI_ACCENT, ANSI_RESET);
    return 0;
}
