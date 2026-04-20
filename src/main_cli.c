#include "cli/cli_app.h"

/*
 * Alignment assumptions for future extensions:
 * - This file is the dedicated CLI entrypoint and must stay separate from any GUI launcher.
 * - The CLI runtime owns its own orchestration through runCliApp().
 * - Future GUI work should add its own main source instead of modifying this entrypoint.
 */

/* Enter the standalone CLI application runtime. */
int main(void) {
    return runCliApp();
}
