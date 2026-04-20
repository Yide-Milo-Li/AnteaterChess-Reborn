#include <assert.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

#include "cli/cli_app.h"

#define CLI_FIXTURE_DIR "tests/fixtures/cli/"
#define CLI_CAPTURE_FILE CLI_FIXTURE_DIR "capture_app.txt"

/* Replace stdin with a deterministic fixture file for one CLI app scenario. */
static void writeFixtureAndRedirect(const char *path, const char *contents) {
    FILE *file = fopen(path, "w");

    assert(file != NULL);
    fputs(contents, file);
    fclose(file);
    assert(freopen(path, "r", stdin) != NULL);
}

/* Capture stdout while the full CLI app scenario runs. */
static int run_and_capture_cli_app(const char *inputPath, const char *input, char *buffer, size_t size) {
    int savedStdout = _dup(_fileno(stdout));
    FILE *file;
    size_t bytesRead;
    int result;

    writeFixtureAndRedirect(inputPath, input);
    assert(savedStdout != -1);
    file = freopen(CLI_CAPTURE_FILE, "w", stdout);
    assert(file != NULL);
    result = runCliApp();
    fflush(stdout);
    assert(_dup2(savedStdout, _fileno(stdout)) != -1);
    _close(savedStdout);

    file = fopen(CLI_CAPTURE_FILE, "r");
    assert(file != NULL);
    bytesRead = fread(buffer, 1, size - 1, file);
    buffer[bytesRead] = '\0';
    fclose(file);
    return result;
}

/* Check that one human-vs-human CLI session can play a move and exit cleanly. */
static void test_cli_app_full_session(void) {
    char buffer[8192];
    int result = run_and_capture_cli_app(
        CLI_FIXTURE_DIR "cli_app_session.txt",
        "1\n1\n2\n1\nE2 E4\n3\n3\n",
        buffer,
        sizeof(buffer)
    );

    assert(result == 0);
    assert(strstr(buffer, "Anteater Chess CLI") != NULL);
    assert(strstr(buffer, "Game Over") != NULL);
}

/* Check that disabled AI modes stay unavailable and return the user to the menu flow. */
static void test_cli_app_disabled_ai_notice(void) {
    char buffer[8192];
    int result = run_and_capture_cli_app(
        CLI_FIXTURE_DIR "cli_app_disabled_ai.txt",
        "1\n2\n4\n2\n",
        buffer,
        sizeof(buffer)
    );

    assert(result == 0);
    assert(strstr(buffer, "AI game modes is currently disabled") != NULL);
}

/* Run the standalone CLI app regression suite. */
int main(void) {
    test_cli_app_full_session();
    test_cli_app_disabled_ai_notice();
    return 0;
}
