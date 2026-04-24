#include <assert.h>
#include <stddef.h>

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "gameplay/endgame.h"
#include "gameplay/execution.h"

/*
 * Alignment assumptions for future extensions:
 * - These tests treat endgame.c as the owner of check, mate, stalemate, and material detection.
 * - Mate/stalemate analysis must stay correct even when legal move generation excludes king captures.
 * - Trial analysis must not depend on move-history storage capacity.
 */

/* Clear the board so each endgame test can install only the pieces it needs. */
void clearBoardForEndgameTest(Board *board) {
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            setPiece(board, createPosition(row, col), createPiece(EMPTY_PIECE, EMPTY_COLOR));
        }
    }
}

/* Check that line attacks are detected and blocked correctly. */
void test_is_in_check_detects_attacks_and_blockers(void) {
    GameState state;

    initGameState(&state, NULL);
    clearBoardForEndgameTest(&state.board);

    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(7, 0), createPiece(ROOK, BLACK));

    assert(isInCheck(&state, WHITE) == 1);

    setPiece(&state.board, createPosition(7, 3), createPiece(BISHOP, WHITE));
    assert(isInCheck(&state, WHITE) == 0);
}

/* Check a basic forced-mate position for the side to move. */
void test_checkmate_detection_finds_forced_mate(void) {
    GameState state;

    initGameState(&state, NULL);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = BLACK;

    setPiece(&state.board, createPosition(0, 0), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(1, 1), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(2, 2), createPiece(KING, WHITE));

    assert(isInCheck(&state, BLACK) == 1);
    assert(isCheckmate(&state, BLACK) == 1);
    assert(isStalemate(&state, BLACK) == 0);
}

/* Check that pseudo "capture king" escapes do not break checkmate detection. */
void test_checkmate_ignores_capture_king_pseudomove(void) {
    GameState state;

    initGameState(&state, NULL);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = BLACK;

    setPiece(&state.board, createPosition(0, 0), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(1, 1), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 2), createPiece(ROOK, WHITE));

    assert(isInCheck(&state, BLACK) == 1);
    assert(isCheckmate(&state, BLACK) == 1);
}

/* Check a position with no legal escape moves but no current check. */
void test_stalemate_detection_finds_no_legal_move_position(void) {
    GameState state;

    initGameState(&state, NULL);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = BLACK;

    setPiece(&state.board, createPosition(0, 0), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(1, 2), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(2, 2), createPiece(KING, WHITE));

    assert(isInCheck(&state, BLACK) == 0);
    assert(isStalemate(&state, BLACK) == 1);
    assert(isCheckmate(&state, BLACK) == 0);
}

/* Check common low-material positions that should be treated as draws. */
void test_insufficient_material_detects_simple_draws(void) {
    GameState state;

    initGameState(&state, NULL);
    clearBoardForEndgameTest(&state.board);

    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    assert(isInsufficientMaterial(&state) == 1);

    setPiece(&state.board, createPosition(4, 4), createPiece(BISHOP, WHITE));
    assert(isInsufficientMaterial(&state) == 1);

    setPiece(&state.board, createPosition(4, 6), createPiece(QUEEN, WHITE));
    assert(isInsufficientMaterial(&state) == 0);
}

/* Check additional low-material combinations near the detector boundaries. */
void test_insufficient_material_detects_boundary_combinations(void) {
    GameState state;

    initGameState(&state, NULL);
    clearBoardForEndgameTest(&state.board);

    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(4, 4), createPiece(KNIGHT, WHITE));
    assert(isInsufficientMaterial(&state) == 1);

    clearBoardForEndgameTest(&state.board);
    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(4, 4), createPiece(KNIGHT, WHITE));
    setPiece(&state.board, createPosition(4, 6), createPiece(KNIGHT, BLACK));
    assert(isInsufficientMaterial(&state) == 1);

    clearBoardForEndgameTest(&state.board);
    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(3, 3), createPiece(BISHOP, WHITE));
    setPiece(&state.board, createPosition(5, 5), createPiece(BISHOP, BLACK));
    assert(isInsufficientMaterial(&state) == 1);

    clearBoardForEndgameTest(&state.board);
    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(3, 3), createPiece(BISHOP, WHITE));
    setPiece(&state.board, createPosition(5, 4), createPiece(BISHOP, BLACK));
    assert(isInsufficientMaterial(&state) == 0);
}

/* Check that detectGameResult writes the expected terminal result into state. */
void test_detect_game_result_updates_game_state(void) {
    GameState state;

    initGameState(&state, NULL);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = BLACK;

    setPiece(&state.board, createPosition(0, 0), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(1, 1), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(2, 2), createPiece(KING, WHITE));

    assert(detectGameResult(&state) == 1);
    assert(getGameResult(&state) == RESULT_WHITE_WIN);
    assert(isGameOver(&state) == 1);

    clearBoardForEndgameTest(&state.board);
    state.currentTurn = BLACK;
    setPiece(&state.board, createPosition(0, 0), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(1, 2), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(2, 2), createPiece(KING, WHITE));

    assert(detectGameResult(&state) == 1);
    assert(getGameResult(&state) == RESULT_DRAW);
    assert(isGameOver(&state) == 1);

    initGameState(&state, NULL);
    assert(detectGameResult(&state) == 0);
    assert(getGameResult(&state) == RESULT_NONE);
    assert(isGameOver(&state) == 0);
}

/* Apply one ordinary move in a repetition test sequence. */
static void apply_test_move(GameState *state, Position from, Position to) {
    Piece piece;
    Move move;

    assert(state != NULL);
    piece = getPiece(&state->board, from);
    move = createMove(from, to, piece);
    assert(applyMove(state, move) == 0);
}

/* Repeat both knights out and back twice, returning to the initial full
 * position for a third occurrence with White to move. */
static void play_knight_repetition_sequence(GameState *state) {
    int cycle;

    for (cycle = 0; cycle < 2; ++cycle) {
        apply_test_move(state, createPosition(7, 1), createPosition(5, 2));
        apply_test_move(state, createPosition(0, 1), createPosition(2, 2));
        apply_test_move(state, createPosition(5, 2), createPosition(7, 1));
        apply_test_move(state, createPosition(2, 2), createPosition(0, 1));
    }
}

static void test_threefold_repetition_detects_repeated_position(void) {
    GameState state;

    initGameState(&state, NULL);
    play_knight_repetition_sequence(&state);

    assert(state.currentTurn == WHITE);
    assert(state.moveHistory.count == 8);
    assert(isThreefoldRepetition(&state) == 1);
}

static void test_threefold_auto_draw_only_in_computer_vs_computer(void) {
    GameConfig config;
    GameState state;

    initGameConfigForMode(&config, MODE_COMPUTER_VS_COMPUTER);
    initGameState(&state, &config);
    play_knight_repetition_sequence(&state);
    assert(detectGameResult(&state) == 1);
    assert(getGameResult(&state) == RESULT_DRAW);

    initGameConfigForMode(&config, MODE_HUMAN_VS_HUMAN);
    initGameState(&state, &config);
    play_knight_repetition_sequence(&state);
    assert(isThreefoldRepetition(&state) == 1);
    assert(detectGameResult(&state) == 0);
    assert(getGameResult(&state) == RESULT_NONE);

    initGameConfigForMode(&config, MODE_HUMAN_VS_COMPUTER);
    initGameState(&state, &config);
    play_knight_repetition_sequence(&state);
    assert(isThreefoldRepetition(&state) == 1);
    assert(detectGameResult(&state) == 0);
    assert(getGameResult(&state) == RESULT_NONE);
}

/* Check that mate detection still works when move history storage is already full. */
void test_endgame_detection_ignores_history_capacity_limit(void) {
    GameState state;

    initGameState(&state, NULL);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = BLACK;
    state.moveHistory.count = MAX_MOVES;
    state.moveCount = MAX_MOVES;

    setPiece(&state.board, createPosition(0, 0), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(1, 1), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(2, 2), createPiece(KING, WHITE));

    assert(isCheckmate(&state, BLACK) == 1);
    assert(detectGameResult(&state) == 1);
    assert(getGameResult(&state) == RESULT_WHITE_WIN);
}

/* Run the endgame regression suite for check, mate, stalemate, and draw logic. */
int main(void) {
    test_is_in_check_detects_attacks_and_blockers();
    test_checkmate_detection_finds_forced_mate();
    test_checkmate_ignores_capture_king_pseudomove();
    test_stalemate_detection_finds_no_legal_move_position();
    test_insufficient_material_detects_simple_draws();
    test_insufficient_material_detects_boundary_combinations();
    test_detect_game_result_updates_game_state();
    test_threefold_repetition_detects_repeated_position();
    test_threefold_auto_draw_only_in_computer_vs_computer();
    test_endgame_detection_ignores_history_capacity_limit();
    return 0;
}
