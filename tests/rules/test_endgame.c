#include <stdlib.h>
#include "anteater/rules.h"
#include <assert.h>
#include <stddef.h>

static int terminal(const AcPosition *p, AcColor side, int check) {
    AcPosition copy = *p;
    copy.currentTurn = side;
    AcMoveList *list = malloc(sizeof(*list));
    assert(list);
    assert(!ac_generate_legal_moves(&copy, list));
    int none = !list->count;
    free(list);
    return none && ac_is_in_check(&copy, side) == check;
}
static int is_checkmate(const AcPosition *p, AcColor side) {
    return terminal(p, side, 1);
}
static int is_stalemate(const AcPosition *p, AcColor side) {
    return terminal(p, side, 0);
}
/*
 * Alignment assumptions for future extensions:
 * - These tests treat endgame.c as the owner of check, mate, stalemate, and material detection.
 * - Mate/stalemate analysis must stay correct even when legal move generation excludes king captures.
 * - Trial analysis must not depend on move-history storage capacity.
 */

/* Clear the board so each endgame test can install only the pieces it needs. */
void clearBoardForEndgameTest(AcBoard *board) {
    int row;
    int col;

    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            ac_set_piece(board, ac_create_position(row, col), ac_create_piece(AC_EMPTY_PIECE, AC_EMPTY_COLOR));
        }
    }
}

/* Check that line attacks are detected and blocked correctly. */
void test_is_in_check_detects_attacks_and_blockers(void) {
    AcPosition state;

    ac_position_init(&state);
    clearBoardForEndgameTest(&state.board);

    ac_set_piece(&state.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(0, 5), ac_create_piece(AC_KING, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(7, 0), ac_create_piece(AC_ROOK, AC_BLACK));

    assert(ac_is_in_check(&state, AC_WHITE) == 1);

    ac_set_piece(&state.board, ac_create_position(7, 3), ac_create_piece(AC_BISHOP, AC_WHITE));
    assert(ac_is_in_check(&state, AC_WHITE) == 0);
}

/* Check a basic forced-mate position for the side to move. */
void test_checkmate_detection_finds_forced_mate(void) {
    AcPosition state;

    ac_position_init(&state);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = AC_BLACK;

    ac_set_piece(&state.board, ac_create_position(0, 0), ac_create_piece(AC_KING, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(1, 1), ac_create_piece(AC_QUEEN, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(2, 2), ac_create_piece(AC_KING, AC_WHITE));

    assert(ac_is_in_check(&state, AC_BLACK) == 1);
    assert(is_checkmate(&state, AC_BLACK) == 1);
    assert(is_stalemate(&state, AC_BLACK) == 0);
}

/* Check that pseudo "capture king" escapes do not break checkmate detection. */
void test_checkmate_ignores_capture_king_pseudomove(void) {
    AcPosition state;

    ac_position_init(&state);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = AC_BLACK;

    ac_set_piece(&state.board, ac_create_position(0, 0), ac_create_piece(AC_KING, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(1, 1), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(0, 2), ac_create_piece(AC_ROOK, AC_WHITE));

    assert(ac_is_in_check(&state, AC_BLACK) == 1);
    assert(is_checkmate(&state, AC_BLACK) == 1);
}

/* Check a position with no legal escape moves but no current check. */
void test_stalemate_detection_finds_no_legal_move_position(void) {
    AcPosition state;

    ac_position_init(&state);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = AC_BLACK;

    ac_set_piece(&state.board, ac_create_position(0, 0), ac_create_piece(AC_KING, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(1, 2), ac_create_piece(AC_QUEEN, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(2, 2), ac_create_piece(AC_KING, AC_WHITE));

    assert(ac_is_in_check(&state, AC_BLACK) == 0);
    assert(is_stalemate(&state, AC_BLACK) == 1);
    assert(is_checkmate(&state, AC_BLACK) == 0);
}

/* Check common low-material positions that should be treated as draws. */
void test_insufficient_material_detects_simple_draws(void) {
    AcPosition state;

    ac_position_init(&state);
    clearBoardForEndgameTest(&state.board);

    ac_set_piece(&state.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(0, 5), ac_create_piece(AC_KING, AC_BLACK));
    assert(ac_is_insufficient_material(&state) == 1);

    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_BISHOP, AC_WHITE));
    assert(ac_is_insufficient_material(&state) == 1);

    ac_set_piece(&state.board, ac_create_position(4, 6), ac_create_piece(AC_QUEEN, AC_WHITE));
    assert(ac_is_insufficient_material(&state) == 0);
}

/* Check additional low-material combinations near the detector boundaries. */
void test_insufficient_material_detects_boundary_combinations(void) {
    AcPosition state;

    ac_position_init(&state);
    clearBoardForEndgameTest(&state.board);

    ac_set_piece(&state.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(0, 5), ac_create_piece(AC_KING, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_KNIGHT, AC_WHITE));
    assert(ac_is_insufficient_material(&state) == 1);

    clearBoardForEndgameTest(&state.board);
    ac_set_piece(&state.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(0, 5), ac_create_piece(AC_KING, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_KNIGHT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(4, 6), ac_create_piece(AC_KNIGHT, AC_BLACK));
    assert(ac_is_insufficient_material(&state) == 1);

    clearBoardForEndgameTest(&state.board);
    ac_set_piece(&state.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(0, 5), ac_create_piece(AC_KING, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(3, 3), ac_create_piece(AC_BISHOP, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(5, 5), ac_create_piece(AC_BISHOP, AC_BLACK));
    assert(ac_is_insufficient_material(&state) == 1);

    clearBoardForEndgameTest(&state.board);
    ac_set_piece(&state.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(0, 5), ac_create_piece(AC_KING, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(3, 3), ac_create_piece(AC_BISHOP, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(5, 4), ac_create_piece(AC_BISHOP, AC_BLACK));
    assert(ac_is_insufficient_material(&state) == 0);
}

int main(void) {
    test_is_in_check_detects_attacks_and_blockers();
    test_checkmate_detection_finds_forced_mate();
    test_checkmate_ignores_capture_king_pseudomove();
    test_stalemate_detection_finds_no_legal_move_position();
    test_insufficient_material_detects_simple_draws();
    test_insufficient_material_detects_boundary_combinations();
    return 0;
}
