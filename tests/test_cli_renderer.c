#include <assert.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

#include "cli/cli_renderer.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "time/clock.h"
#include "turn/turn_timer.h"

#define CLI_FIXTURE_DIR "tests/fixtures/cli/"
#define CLI_CAPTURE_FILE CLI_FIXTURE_DIR "capture_renderer.txt"

/* Capture stdout to a fixture file while one renderer helper writes to it. */
static void captureStdout(void (*fn)(void *), void *context, const char *path, char *buffer, size_t size) {
    int savedStdout = _dup(_fileno(stdout));
    FILE *file;
    size_t bytesRead;

    assert(savedStdout != -1);
    file = freopen(path, "w", stdout);
    assert(file != NULL);
    fn(context);
    fflush(stdout);
    assert(_dup2(savedStdout, _fileno(stdout)) != -1);
    _close(savedStdout);

    file = fopen(path, "r");
    assert(file != NULL);
    bytesRead = fread(buffer, 1, size - 1, file);
    buffer[bytesRead] = '\0';
    fclose(file);
}

typedef struct {
    const GameState *state;
} RenderContext;

/* Adapter that lets the generic capture helper call cliRenderBoard. */
static void render_board_wrapper(void *context) {
    RenderContext *renderContext = (RenderContext *)context;

    assert(cliRenderBoard(renderContext->state) == 0);
}

/* Adapter that lets the generic capture helper call cliDisplayGameStatus. */
static void render_status_wrapper(void *context) {
    RenderContext *renderContext = (RenderContext *)context;

    assert(cliDisplayGameStatus(renderContext->state) == 0);
}

/* Build a default game state for CLI rendering tests. */
static GameState create_state(int timerEnabled, int initialTimeSeconds) {
    GameConfig config;
    GameState state;

    initDefaultGameConfig(&config);
    config.timerEnabled = timerEnabled;
    config.initialTimeSeconds = initialTimeSeconds;
    initGameState(&state, &config);
    return state;
}

/* Check that the board renderer prints the expected labels and piece tokens. */
static void test_board_renderer_output(void) {
    GameState state = create_state(0, 0);
    RenderContext context = {&state};
    char buffer[4096];

    captureStdout(render_board_wrapper, &context, CLI_CAPTURE_FILE, buffer, sizeof(buffer));
    assert(strstr(buffer, "\x1b[") != NULL);
    assert(strstr(buffer, "Board") != NULL);
    assert(strstr(buffer, "A") != NULL);
    assert(strstr(buffer, "J") != NULL);
    assert(strstr(buffer, "\u250C") != NULL);
    assert(strstr(buffer, "\u253C") != NULL);
    assert(strchr(buffer, 'r') != NULL);
    assert(strchr(buffer, 'R') != NULL);
    assert(strstr(buffer, "Legend:") != NULL);
}

/* Check that the status block prints turn and timer details. */
static void test_status_output(void) {
    GameState state = create_state(1, 30);
    RenderContext context = {&state};
    char buffer[4096];

    assert(initClock() == 0);
    assert(initTurnTimer(&state) == 0);
    captureStdout(render_status_wrapper, &context, CLI_CAPTURE_FILE, buffer, sizeof(buffer));
    assert(strstr(buffer, "\x1b[") != NULL);
    assert(strstr(buffer, "Game Status") != NULL);
    assert(strstr(buffer, "Current Turn") != NULL);
    assert(strstr(buffer, "Elapsed Time") != NULL);
    assert(strstr(buffer, "Turn Timer") != NULL);
    assert(strstr(buffer, "White Timer") != NULL);
    assert(strstr(buffer, "Black Timer") != NULL);
    assert(strstr(buffer, "00:00:30") != NULL);
}

/* Run the CLI renderer regression suite. */
int main(void) {
    test_board_renderer_output();
    test_status_output();
    return 0;
}
