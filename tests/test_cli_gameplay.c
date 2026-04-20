#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define dup_fd _dup
#define dup2_fd _dup2
#define close_fd _close
#define fileno_fd _fileno
#else
#include <unistd.h>
#define dup_fd dup
#define dup2_fd dup2
#define close_fd close
#define fileno_fd fileno
#endif

#include "cli/cli_feedback.h"
#include "cli/cli_gameplay.h"
#include "error/error.h"

#define CLI_FIXTURE_DIR "tests/fixtures/cli/"
#define CLI_CAPTURE_FILE "bin/tests/test_cli_gameplay_capture.txt"

/* Replace stdin with a deterministic fixture file for one CLI input test. */
static void writeFixtureAndRedirect(const char *path, const char *contents) {
    FILE *file = fopen(path, "w");

    assert(file != NULL);
    fputs(contents, file);
    fclose(file);
    assert(freopen(path, "r", stdin) != NULL);
}

/* Capture stdout to a fixture file while one CLI rendering helper runs. */
static void captureStdout(void (*fn)(void), const char *path, char *buffer, size_t size) {
    int savedStdout = dup_fd(fileno_fd(stdout));
    FILE *file;
    size_t bytesRead;

    assert(savedStdout != -1);
    file = freopen(path, "w", stdout);
    assert(file != NULL);
    fn();
    fflush(stdout);
    assert(dup2_fd(savedStdout, fileno_fd(stdout)) != -1);
    close_fd(savedStdout);

    file = fopen(path, "r");
    assert(file != NULL);
    bytesRead = fread(buffer, 1, size - 1, file);
    buffer[bytesRead] = '\0';
    fclose(file);
}

/* Provide a wrapper so the generic stdout-capture helper can call the hint API. */
static void show_hint_wrapper(void) {
    assert(cliShowMoveFormatHint() == 0);
}

/* Provide a wrapper so stdout capture can verify shared CLI error rendering. */
static void show_error_wrapper(void) {
    assert(cliShowErrorMessage(ERR_ILLEGAL_MOVE) == 0);
}

/* Provide a wrapper so stdout capture can verify the human-turn action menu. */
static void show_action_menu_wrapper(void) {
    int selection;

    assert(cliGetGameplayAction(&selection) == 0);
    assert(selection == 5);
}

/* Check that gameplay action selection reprompts until it receives a valid choice. */
static void test_gameplay_action_selection(void) {
    int selection;

    writeFixtureAndRedirect(CLI_FIXTURE_DIR "gameplay_action.txt", "9\n5\n");
    assert(cliGetGameplayAction(&selection) == 0);
    assert(selection == 5);
}

/* Check that gameplay move parsing reuses the shared command parser contract. */
static void test_gameplay_move_command(void) {
    Command command;

    writeFixtureAndRedirect(CLI_FIXTURE_DIR "gameplay_move_valid.txt", " e2   E4 \n");
    assert(cliGetMoveCommand(&command) == 0);
    assert(command.type == CMD_MOVE);
    assert(command.from.row == 6 && command.from.col == 4);
    assert(command.to.row == 4 && command.to.col == 4);

    writeFixtureAndRedirect(CLI_FIXTURE_DIR "gameplay_move_invalid.txt", "Z9 E4\n");
    assert(cliGetMoveCommand(&command) != 0);
}

/* Check that the gameplay action menu advertises AI suggestions on human turns. */
static void test_gameplay_action_menu_output(void) {
    char buffer[512];

    writeFixtureAndRedirect(CLI_FIXTURE_DIR "gameplay_action.txt", "5\n");
    captureStdout(show_action_menu_wrapper, CLI_CAPTURE_FILE, buffer, sizeof(buffer));
    assert(strstr(buffer, "Actions") != NULL);
    assert(strstr(buffer, "Show AI suggestion") != NULL);
}

/* Check that the move-format hint prints the documented coordinate example. */
static void test_move_format_hint_output(void) {
    char buffer[256];

    captureStdout(show_hint_wrapper, CLI_CAPTURE_FILE, buffer, sizeof(buffer));
    assert(strstr(buffer, "Move format") != NULL);
    assert(strstr(buffer, "Example: E2 E4") != NULL);
    assert(strstr(buffer, "\x1b[") != NULL);
}

/* Check that CLI error output delegates its message text to the shared error module. */
static void test_cli_error_output_uses_shared_error_message(void) {
    char buffer[256];

    captureStdout(show_error_wrapper, CLI_CAPTURE_FILE, buffer, sizeof(buffer));
    assert(strstr(buffer, "[Error]") != NULL);
    assert(strstr(buffer, getErrorMessage(ERR_ILLEGAL_MOVE)) != NULL);
}

/* Run the CLI gameplay regression suite. */
int main(void) {
    test_gameplay_action_selection();
    test_gameplay_move_command();
    test_gameplay_action_menu_output();
    test_move_format_hint_output();
    test_cli_error_output_uses_shared_error_message();
    return 0;
}
