#include "cli/cli_app.h"

#ifdef _WIN32
#include <windows.h>
#endif

/*
 * Alignment assumptions for future extensions:
 * - This file is the dedicated CLI entrypoint and must stay separate from any GUI launcher.
 * - The CLI remains a temporary frontend, but runCliApp() now drives gameplay
 *   through the public controller layer instead of calling the FSM directly.
 * - Future GUI work should add its own main source instead of modifying this entrypoint.
 */

/* Enable common Windows terminal features so standard ANSI styling renders reliably. */
static void initialize_cli_console(void) {
#ifdef _WIN32
    HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;

    if (outputHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    if (!GetConsoleMode(outputHandle, &mode)) {
        return;
    }

    (void)SetConsoleMode(outputHandle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

/* Enter the temporary standalone CLI application runtime. */
int main(void) {
    initialize_cli_console();
    return runCliApp();
}
