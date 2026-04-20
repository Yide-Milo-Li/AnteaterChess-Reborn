#include <assert.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

#include "cli/cli_app.h"

#define CLI_FIXTURE_DIR "tests/fixtures/cli/"
#define CLI_TEMP_INPUT_FILE "build/test_cli_app_input.txt"
#define CLI_CAPTURE_FILE "build/test_cli_app_capture.txt"

/* Replace stdin with a deterministic temporary input file for one CLI app scenario. */
static void writeInputAndRedirect(const char *path, const char *contents) {
    FILE *file = fopen(path, "w");

    assert(file != NULL);
    fputs(contents, file);
    fclose(file);
    assert(freopen(path, "r", stdin) != NULL);
}

/* Capture stdout while the full CLI app scenario runs. */
static int run_and_capture_cli_app(const char *input, char *buffer, size_t size) {
    int savedStdout = _dup(_fileno(stdout));
    FILE *file;
    size_t bytesRead;
    int result;

    writeInputAndRedirect(CLI_TEMP_INPUT_FILE, input);
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

/* Count how many times one literal substring appears in captured output. */
static int count_occurrences(const char *buffer, const char *needle) {
    int count = 0;
    const char *cursor = buffer;
    size_t needleLength;

    assert(buffer != NULL);
    assert(needle != NULL);

    needleLength = strlen(needle);
    while ((cursor = strstr(cursor, needle)) != NULL) {
        ++count;
        cursor += needleLength;
    }

    return count;
}

/* Check that one human-vs-human CLI session can play a move and exit cleanly. */
static void test_cli_app_full_session(void) {
    char buffer[32768];
    int result = run_and_capture_cli_app(
        "1\n1\n2\n1\nE2 E4\n3\n3\n",
        buffer,
        sizeof(buffer)
    );

    assert(result == 0);
    assert(strstr(buffer, "Anteater Chess CLI") != NULL);
    assert(strstr(buffer, "Gameplay View") != NULL);
    assert(strstr(buffer, "WHITE TO MOVE") != NULL);
    assert(strstr(buffer, "Game Status") != NULL);
    assert(strstr(buffer, "Board") != NULL);
    assert(strstr(buffer, "Actions") != NULL);
    assert(strstr(buffer, "Game Over") != NULL);
}

/* Check that human-vs-computer setup, hint output, and AI replies all work together. */
static void test_cli_app_human_vs_computer_white_session(void) {
    char buffer[32768];
    int result = run_and_capture_cli_app(
        "1\n2\n1\n1\n1\n2\n5\n1\nE2 E4\n3\n3\n",
        buffer,
        sizeof(buffer)
    );

    assert(result == 0);
    assert(strstr(buffer, "Selected mode: Human vs Computer") != NULL);
    assert(strstr(buffer, "Human Side: White") != NULL);
    assert(strstr(buffer, "Show AI suggestion") != NULL);
    assert(strstr(buffer, "[Hint]") != NULL);
    assert(strstr(buffer, "[AI Move]") != NULL);
    assert(strstr(buffer, "Human vs Computer (Disabled)") == NULL);
    assert(strstr(buffer, "AI game modes is currently disabled") == NULL);
    assert(count_occurrences(buffer, "WHITE TO MOVE") >= 2);
}

/* Check that black-side human games auto-play White's first AI move. */
static void test_cli_app_human_vs_computer_black_session(void) {
    char buffer[32768];
    int result = run_and_capture_cli_app(
        "1\n2\n2\n1\n1\n2\n3\n3\n",
        buffer,
        sizeof(buffer)
    );

    assert(result == 0);
    assert(strstr(buffer, "Selected mode: Human vs Computer") != NULL);
    assert(strstr(buffer, "Human Side: Black") != NULL);
    assert(strstr(buffer, "[AI]") != NULL);
    assert(strstr(buffer, "White is thinking") != NULL);
    assert(strstr(buffer, "[AI Move]") != NULL);
    assert(strstr(buffer, "White Knight") != NULL);
    assert(strstr(buffer, "Actions") != NULL);
}

/* Run the standalone CLI app regression suite. */
int main(void) {
    test_cli_app_full_session();
    test_cli_app_human_vs_computer_white_session();
    test_cli_app_human_vs_computer_black_session();
    return 0;
}
