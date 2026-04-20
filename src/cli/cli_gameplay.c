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

/* Read one validated gameplay action selection from the CLI action menu. */
int cliGetGameplayAction(int *selection) {
    if (selection == NULL) {
        return 1;
    }

    printf("1. Make move\n");
    printf("2. Undo\n");
    printf("3. Leave game\n");
    printf("4. Exit program\n");
    printf("5. Show move format hint\n");

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

    printf("Enter move as: FROM TO (example: E2 E4)\n");
    return getMoveInput(cmd);
}

/* Print one stable move-format hint for the CLI gameplay prompt. */
int cliShowMoveFormatHint(void) {
    printf("Move format: enter two coordinates separated by spaces, such as E2 E4.\n");
    printf("Coordinates are case-insensitive and may have leading or trailing whitespace.\n");
    return 0;
}
