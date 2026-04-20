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

#define CAPTURE_FILE "bin/tests/test_boarddisplay_capture.txt"

/* Clear the board so the renderer output only reflects the pieces under test. */
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

/* Build one renderer-ready state with a custom board layout. */
static GameState create_test_state(void) {
    GameConfig config;
    GameState state;

    initDefaultGameConfig(&config);
    initGameState(&state, &config);
    clear_board(&state.board);
    state.systemState = GAMEPLAY_STATE;
    return state;
}

/* Capture stdout so the test can assert the board renderer's text output. */
static void capture_stdout(void (*fn)(void *), void *context,
                           const char *path, char *buffer, size_t size) {
    int saved_stdout = dup_fd(fileno_fd(stdout));
    FILE *file;
    size_t bytes_read;

    assert(saved_stdout != -1);
    file = freopen(path, "w", stdout);
    assert(file != NULL);
    fn(context);
    fflush(stdout);
    assert(dup2_fd(saved_stdout, fileno_fd(stdout)) != -1);
    close_fd(saved_stdout);

    file = fopen(path, "r");
    assert(file != NULL);
    bytes_read = fread(buffer, 1, size - 1, file);
    buffer[bytes_read] = '\0';
    fclose(file);
}

/* Remove ANSI color/style escapes so assertions can compare plain board lines. */
static void strip_ansi_sequences(const char *input, char *output, size_t size) {
    size_t write_index = 0;
    size_t read_index = 0;

    assert(size > 0);

    while (input[read_index] != '\0' && write_index + 1 < size) {
        if (input[read_index] == '\x1b' && input[read_index + 1] == '[') {
            read_index += 2;
            while (input[read_index] != '\0'
                   && (input[read_index] < '@' || input[read_index] > '~')) {
                ++read_index;
            }
            if (input[read_index] != '\0') {
                ++read_index;
            }
            continue;
        }

        output[write_index] = input[read_index];
        ++write_index;
        ++read_index;
    }

    output[write_index] = '\0';
}

/* Extract one rendered board row so the test can compare its cell contents. */
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

typedef struct {
    const GameState *state;
} RenderContext;

/* Adapter that lets the generic capture helper call cliRenderBoard. */
static void render_board_wrapper(void *context) {
    RenderContext *render_context = (RenderContext *)context;

    assert(cliRenderBoard(render_context->state) == 0);
}

/* Verify the board display reflects a manually modified board state. */
static void test_board_display_reflects_modified_board_contents(void) {
    GameState state = create_test_state();
    RenderContext context = {&state};
    char rendered[4096];
    char stripped[4096];
    char row8[128];
    char row4[128];
    char row2[128];

    setPiece(&state.board, createPosition(0, 9), createPiece(ROOK, BLACK));
    setPiece(&state.board, createPosition(4, 4), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(6, 1), createPiece(ANTEATER, WHITE));

    capture_stdout(render_board_wrapper, &context, CAPTURE_FILE,
                   rendered, sizeof(rendered));
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

int main(void) {
    test_board_display_reflects_modified_board_contents();
    return 0;
}
