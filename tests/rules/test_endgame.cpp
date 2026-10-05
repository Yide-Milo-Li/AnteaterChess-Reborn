#include <stdlib.h>
#include "anteater/rules.hpp"
#include <assert.h>
#include <stddef.h>

using namespace ac;

static int terminal(const Position *p, Color side, int check) {
    Position copy = *p;
    copy.currentTurn = side;
    MoveList *list = static_cast<MoveList *>(malloc(sizeof(*list)));
    assert(list);
    assert(!generate_legal_moves(&copy, list));
    int none = !list->count;
    free(list);
    return none && is_in_check(&copy, side) == check;
}
static int is_checkmate(const Position *p, Color side) {
    return terminal(p, side, 1);
}
static int is_stalemate(const Position *p, Color side) {
    return terminal(p, side, 0);
}
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

    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            set_piece(board, create_position(row, col), create_piece(PieceType::Empty, Color::Empty));
        }
    }
}

/* Check that line attacks are detected and blocked correctly. */
void test_is_in_check_detects_attacks_and_blockers(void) {
    Position state;

    position_init(&state);
    clearBoardForEndgameTest(&state.board);

    set_piece(&state.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&state.board, create_position(0, 5), create_piece(PieceType::King, Color::Black));
    set_piece(&state.board, create_position(7, 0), create_piece(PieceType::Rook, Color::Black));

    assert(is_in_check(&state, Color::White) == 1);

    set_piece(&state.board, create_position(7, 3), create_piece(PieceType::Bishop, Color::White));
    assert(is_in_check(&state, Color::White) == 0);
}

/* Check a basic forced-mate position for the side to move. */
void test_checkmate_detection_finds_forced_mate(void) {
    Position state;

    position_init(&state);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = Color::Black;

    set_piece(&state.board, create_position(0, 0), create_piece(PieceType::King, Color::Black));
    set_piece(&state.board, create_position(1, 1), create_piece(PieceType::Queen, Color::White));
    set_piece(&state.board, create_position(2, 2), create_piece(PieceType::King, Color::White));

    assert(is_in_check(&state, Color::Black) == 1);
    assert(is_checkmate(&state, Color::Black) == 1);
    assert(is_stalemate(&state, Color::Black) == 0);
}

/* Check that pseudo "capture king" escapes do not break checkmate detection. */
void test_checkmate_ignores_capture_king_pseudomove(void) {
    Position state;

    position_init(&state);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = Color::Black;

    set_piece(&state.board, create_position(0, 0), create_piece(PieceType::King, Color::Black));
    set_piece(&state.board, create_position(1, 1), create_piece(PieceType::King, Color::White));
    set_piece(&state.board, create_position(0, 2), create_piece(PieceType::Rook, Color::White));

    assert(is_in_check(&state, Color::Black) == 1);
    assert(is_checkmate(&state, Color::Black) == 1);
}

/* Check a position with no legal escape moves but no current check. */
void test_stalemate_detection_finds_no_legal_move_position(void) {
    Position state;

    position_init(&state);
    clearBoardForEndgameTest(&state.board);
    state.currentTurn = Color::Black;

    set_piece(&state.board, create_position(0, 0), create_piece(PieceType::King, Color::Black));
    set_piece(&state.board, create_position(1, 2), create_piece(PieceType::Queen, Color::White));
    set_piece(&state.board, create_position(2, 2), create_piece(PieceType::King, Color::White));

    assert(is_in_check(&state, Color::Black) == 0);
    assert(is_stalemate(&state, Color::Black) == 1);
    assert(is_checkmate(&state, Color::Black) == 0);
}

/* Check common low-material positions that should be treated as draws. */
void test_insufficient_material_detects_simple_draws(void) {
    Position state;

    position_init(&state);
    clearBoardForEndgameTest(&state.board);

    set_piece(&state.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&state.board, create_position(0, 5), create_piece(PieceType::King, Color::Black));
    assert(is_insufficient_material(&state) == 1);

    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Bishop, Color::White));
    assert(is_insufficient_material(&state) == 1);

    set_piece(&state.board, create_position(4, 6), create_piece(PieceType::Queen, Color::White));
    assert(is_insufficient_material(&state) == 0);
}

/* Check additional low-material combinations near the detector boundaries. */
void test_insufficient_material_detects_boundary_combinations(void) {
    Position state;

    position_init(&state);
    clearBoardForEndgameTest(&state.board);

    set_piece(&state.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&state.board, create_position(0, 5), create_piece(PieceType::King, Color::Black));
    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Knight, Color::White));
    assert(is_insufficient_material(&state) == 1);

    clearBoardForEndgameTest(&state.board);
    set_piece(&state.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&state.board, create_position(0, 5), create_piece(PieceType::King, Color::Black));
    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Knight, Color::White));
    set_piece(&state.board, create_position(4, 6), create_piece(PieceType::Knight, Color::Black));
    assert(is_insufficient_material(&state) == 1);

    clearBoardForEndgameTest(&state.board);
    set_piece(&state.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&state.board, create_position(0, 5), create_piece(PieceType::King, Color::Black));
    set_piece(&state.board, create_position(3, 3), create_piece(PieceType::Bishop, Color::White));
    set_piece(&state.board, create_position(5, 5), create_piece(PieceType::Bishop, Color::Black));
    assert(is_insufficient_material(&state) == 1);

    clearBoardForEndgameTest(&state.board);
    set_piece(&state.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&state.board, create_position(0, 5), create_piece(PieceType::King, Color::Black));
    set_piece(&state.board, create_position(3, 3), create_piece(PieceType::Bishop, Color::White));
    set_piece(&state.board, create_position(5, 4), create_piece(PieceType::Bishop, Color::Black));
    assert(is_insufficient_material(&state) == 0);
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
