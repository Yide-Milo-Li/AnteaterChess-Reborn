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

#include "cli/cli_renderer.h"
#include "core/board.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "time/clock.h"
#include "turn/turn_timer.h"

#define CLI_FIXTURE_DIR "tests/fixtures/cli/"
#define CLI_CAPTURE_FILE "bin/tests/test_cli_renderer_capture.txt"

/* Capture stdout to a fixture file while one renderer helper writes to it. */
static void captureStdout(void (*fn)(void *), void *context, const char *path, char *buffer, size_t size) {
    int savedStdout = dup_fd(fileno_fd(stdout));
    FILE *file;
    size_t bytesRead;

    assert(savedStdout != -1);
    file = freopen(path, "w", stdout);
    assert(file != NULL);
    fn(context);
    fflush(stdout);
    assert(dup2_fd(savedStdout, fileno_fd(stdout)) != -1);
    close_fd(savedStdout);

    file = fopen(path, "r");
    assert(file != NULL);
    bytesRead = fread(buffer, 1, size - 1, file);
    buffer[bytesRead] = '\0';
    fclose(file);
}

typedef struct {
    const GameState *state;
} RenderContext;

/* Clear the board so focused renderer checks only reflect the pieces under
 * test. */
static void clear_board(Board *board) {
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            setPiece(board, createPosition(row, col),
                     createPiece(EMPTY_PIECE, EMPTY_COLOR));
        }
    }
}

/* Remove ANSI color/style escapes so assertions can compare exact board rows. */
static void strip_ansi_sequences(const char *input, char *output, size_t size) {
    size_t writeIndex = 0;
    size_t readIndex = 0;

    assert(size > 0);

    while (input[readIndex] != '\0' && writeIndex + 1 < size) {
        if (input[readIndex] == '\x1b' && input[readIndex + 1] == '[') {
            readIndex += 2;
            while (input[readIndex] != '\0'
                   && (input[readIndex] < '@' || input[readIndex] > '~')) {
                ++readIndex;
            }
            if (input[readIndex] != '\0') {
                ++readIndex;
            }
            continue;
        }

        output[writeIndex] = input[readIndex];
        ++writeIndex;
        ++readIndex;
    }

    output[writeIndex] = '\0';
}

/* Extract one rendered board line for exact string comparison. */
static void extract_line(const char *buffer, const char *needle,
                         char *line, size_t size) {
    const char *start = strstr(buffer, needle);
    const char *end;
    size_t length;

    assert(start != NULL);
    end = strchr(start, '\n');
    if (end == NULL) {
        end = start + strlen(start);
    }

    length = (size_t)(end - start);
    if (length >= size) {
        length = size - 1;
    }

    memcpy(line, start, length);
    line[length] = '\0';
}

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

/* Check that the board renderer reflects a custom board layout and keeps the
 * documented labels/styles intact. */
static void test_board_renderer_reflects_modified_board_contents(void) {
    GameState state = create_state(0, 0);
    RenderContext context = {&state};
    char rendered[4096];
    char stripped[4096];
    char row8[128];
    char row4[128];
    char row2[128];

    clear_board(&state.board);
    state.systemState = GAMEPLAY_STATE;
    setPiece(&state.board, createPosition(0, 9), createPiece(ROOK, BLACK));
    setPiece(&state.board, createPosition(4, 4), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(6, 1), createPiece(ANTEATER, WHITE));

    captureStdout(render_board_wrapper, &context, CLI_CAPTURE_FILE, rendered, sizeof(rendered));
    assert(strstr(rendered, "\x1b[") != NULL);
    strip_ansi_sequences(rendered, stripped, sizeof(stripped));
    extract_line(stripped, "  8 |", row8, sizeof(row8));
    extract_line(stripped, "  4 |", row4, sizeof(row4));
    extract_line(stripped, "  2 |", row2, sizeof(row2));

    assert(strstr(stripped, "      Board") != NULL);
    assert(strstr(stripped, "      A   B   C   D   E   F   G   H   I   J  ") != NULL);
    assert(strstr(stripped, "Legend: White = uppercase cyan, Black = lowercase gold.") != NULL);
    assert(strcmp(row8, "  8 |   |   |   |   |   |   |   |   |   | r |  8") == 0);
    assert(strcmp(row4, "  4 |   |   |   |   | Q |   |   |   |   |   |  4") == 0);
    assert(strcmp(row2, "  2 |   | E |   |   |   |   |   |   |   |   |  2") == 0);
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
    test_board_renderer_reflects_modified_board_contents();
    test_status_output();
    return 0;
}
