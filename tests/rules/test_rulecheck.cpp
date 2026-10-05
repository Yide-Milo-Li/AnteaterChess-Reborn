#include "anteater/rules.h"
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>

/* Clear the whole board so each test can build one focused validation case. */
static void clear_board(AcBoard *board) {
    int row;
    int col;

    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            ac_set_piece(board, ac_create_position(row, col), ac_create_piece(AC_EMPTY_PIECE, AC_EMPTY_COLOR));
        }
    }
}

/* Build a minimal game state for standalone rule-checking tests. */
static AcPosition create_test_state(AcColor turn) {
    AcGameConfig config;
    AcPosition state;

    ac_init_default_game_config(&config);
    ac_position_init(&state);
    clear_board(&state.board);
    state.currentTurn = turn;
    return state;
}

/* Record one historical move without mutating the current board state. */
static void push_history_move(AcPosition *state, AcMove move) {
    state->enPassant =
        move.movedPiece.type == AC_ANT && abs(move.to.row - move.from.row) == 2 ? move.to : ac_create_position(-1, -1);
    if (move.movedPiece.type == AC_KING)
        state->castlingRights &= ~(3 << (move.movedPiece.color == AC_WHITE ? 0 : 2));
    if (move.movedPiece.type == AC_ROOK && move.from.col == 9)
        state->castlingRights &= ~(1 << (move.movedPiece.color == AC_WHITE ? 0 : 2));
    if (move.movedPiece.type == AC_ROOK && move.from.col == 0)
        state->castlingRights &= ~(2 << (move.movedPiece.color == AC_WHITE ? 0 : 2));
    ++state->moveCount;
}

/* Verify selection results differentiate empty, opponent, and invalid squares. */
static void test_validate_selection_reports_expected_result_codes(void) {
    AcPosition state = create_test_state(AC_WHITE);

    ac_set_piece(&state.board, ac_create_position(6, 4), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(1, 4), ac_create_piece(AC_ROOK, AC_BLACK));

    assert(ac_validate_selection(&state, ac_create_position(-1, 0)) == AC_SELECT_OUT_OF_BOUNDS);
    assert(ac_validate_selection(&state, ac_create_position(4, 4)) == AC_SELECT_EMPTY);
    assert(ac_validate_selection(&state, ac_create_position(6, 4)) == AC_SELECT_VALID);
    assert(ac_validate_selection(&state, ac_create_position(1, 4)) == AC_SELECT_OPPONENT_PIECE);
}

/* Verify the rule checker accepts one ordinary legal move request. */
static void test_validate_move_accepts_basic_ant_advance(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMove move;

    ac_set_piece(&state.board, ac_create_position(6, 4), ac_create_piece(AC_ANT, AC_WHITE));

    move = ac_create_move(ac_create_position(6, 4), ac_create_position(5, 4), ac_create_piece(AC_ANT, AC_WHITE));
    assert(ac_validate_move(&state, move) == 1);
}

/* Verify off-board targets and non-playable origins are rejected cleanly. */
static void test_validate_move_rejects_invalid_origin_and_target_inputs(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMove emptySquareMove;
    AcMove opponentPieceMove;
    AcMove outOfBoundsMove;

    ac_set_piece(&state.board, ac_create_position(6, 4), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(1, 4), ac_create_piece(AC_ANT, AC_BLACK));

    emptySquareMove =
        ac_create_move(ac_create_position(4, 4), ac_create_position(3, 4), ac_create_piece(AC_ANT, AC_WHITE));
    opponentPieceMove =
        ac_create_move(ac_create_position(1, 4), ac_create_position(2, 4), ac_create_piece(AC_ANT, AC_BLACK));
    outOfBoundsMove =
        ac_create_move(ac_create_position(6, 4), ac_create_position(6, 10), ac_create_piece(AC_ANT, AC_WHITE));

    assert(ac_validate_move(&state, emptySquareMove) == 0);
    assert(ac_validate_move(&state, opponentPieceMove) == 0);
    assert(ac_validate_move(&state, outOfBoundsMove) == 0);
}

/* Verify blocked sliding moves are not treated as legal chess moves. */
static void test_validate_move_rejects_blocked_rook_path(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMove blockedMove;

    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_ROOK, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(4, 6), ac_create_piece(AC_ANT, AC_WHITE));

    blockedMove =
        ac_create_move(ac_create_position(4, 4), ac_create_position(4, 7), ac_create_piece(AC_ROOK, AC_WHITE));
    assert(ac_validate_move(&state, blockedMove) == 0);
}

/* Verify explicit capture metadata must match the generated legal move exactly. */
static void test_validate_move_checks_explicit_capture_metadata(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMove partialRequest;
    AcMove exactCaptureRequest;
    AcMove wrongCaptureRequest;

    ac_set_piece(&state.board, ac_create_position(6, 4), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(5, 5), ac_create_piece(AC_KNIGHT, AC_BLACK));

    partialRequest =
        ac_create_move(ac_create_position(6, 4), ac_create_position(5, 5), ac_create_piece(AC_ANT, AC_WHITE));

    exactCaptureRequest =
        ac_create_move(ac_create_position(6, 4), ac_create_position(5, 5), ac_create_piece(AC_ANT, AC_WHITE));
    ac_add_capture(&exactCaptureRequest, ac_create_position(5, 5), ac_create_piece(AC_KNIGHT, AC_BLACK));

    wrongCaptureRequest =
        ac_create_move(ac_create_position(6, 4), ac_create_position(5, 5), ac_create_piece(AC_ANT, AC_WHITE));
    ac_add_capture(&wrongCaptureRequest, ac_create_position(5, 5), ac_create_piece(AC_BISHOP, AC_BLACK));

    assert(ac_validate_move(&state, partialRequest) == 1);
    assert(ac_validate_move(&state, exactCaptureRequest) == 1);
    assert(ac_validate_move(&state, wrongCaptureRequest) == 0);
}

/* Verify legal filtering rejects moves that would expose the moving side's
 * king or move the king onto an attacked square. */
static void test_validate_move_rejects_self_check_positions(void) {
    AcPosition pinnedState = create_test_state(AC_WHITE);
    AcPosition kingStepState = create_test_state(AC_WHITE);
    AcMove move;

    ac_set_piece(&pinnedState.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&pinnedState.board, ac_create_position(7, 4), ac_create_piece(AC_ROOK, AC_WHITE));
    ac_set_piece(&pinnedState.board, ac_create_position(7, 0), ac_create_piece(AC_ROOK, AC_BLACK));
    ac_set_piece(&pinnedState.board, ac_create_position(0, 5), ac_create_piece(AC_KING, AC_BLACK));
    move = ac_create_move(ac_create_position(7, 4), ac_create_position(6, 4), ac_create_piece(AC_ROOK, AC_WHITE));
    assert(ac_validate_move(&pinnedState, move) == 0);

    ac_set_piece(&kingStepState.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&kingStepState.board, ac_create_position(0, 0), ac_create_piece(AC_KING, AC_BLACK));
    ac_set_piece(&kingStepState.board, ac_create_position(5, 4), ac_create_piece(AC_ROOK, AC_BLACK));
    move = ac_create_move(ac_create_position(7, 5), ac_create_position(6, 4), ac_create_piece(AC_KING, AC_WHITE));
    assert(ac_validate_move(&kingStepState, move) == 0);
}

/* Verify special move validation accepts castling, en passant, and both
 * explicit and implicit promotion requests. */
static void test_validate_move_accepts_supported_special_moves(void) {
    AcPosition promotionState = create_test_state(AC_WHITE);
    AcPosition castlingState = create_test_state(AC_WHITE);
    AcPosition enPassantState = create_test_state(AC_WHITE);
    AcMove move;

    ac_set_piece(&promotionState.board, ac_create_position(1, 2), ac_create_piece(AC_ANT, AC_WHITE));
    move = ac_create_move(ac_create_position(1, 2), ac_create_position(0, 2), ac_create_piece(AC_ANT, AC_WHITE));
    assert(ac_validate_move(&promotionState, move) == 1);
    ac_set_special_move(&move, AC_PROMOTION_ROOK);
    assert(ac_validate_move(&promotionState, move) == 1);

    ac_set_piece(&castlingState.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&castlingState.board, ac_create_position(7, 9), ac_create_piece(AC_ROOK, AC_WHITE));
    ac_set_piece(&castlingState.board, ac_create_position(0, 5), ac_create_piece(AC_KING, AC_BLACK));
    move = ac_create_move(ac_create_position(7, 5), ac_create_position(7, 7), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_special_move(&move, AC_CASTLING_KINGSIDE);
    assert(ac_validate_move(&castlingState, move) == 1);

    ac_set_piece(&enPassantState.board, ac_create_position(3, 4), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&enPassantState.board, ac_create_position(3, 5), ac_create_piece(AC_ANT, AC_BLACK));
    push_history_move(&enPassantState, ac_create_move(ac_create_position(1, 5), ac_create_position(3, 5),
                                                      ac_create_piece(AC_ANT, AC_BLACK)));
    move = ac_create_move(ac_create_position(3, 4), ac_create_position(2, 5), ac_create_piece(AC_ANT, AC_WHITE));
    ac_add_capture(&move, ac_create_position(3, 5), ac_create_piece(AC_ANT, AC_BLACK));
    ac_set_special_move(&move, AC_EN_PASSANT);
    assert(ac_validate_move(&enPassantState, move) == 1);
}

/* Verify explicit anteater recursion metadata is accepted when it matches a
 * generated legal turning chain exactly. */
static void test_validate_move_accepts_recursive_anteater_capture(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMove move;

    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_ANTEATER, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(3, 5), ac_create_piece(AC_ANT, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(3, 6), ac_create_piece(AC_ANT, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(4, 6), ac_create_piece(AC_ANT, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(3, 7), ac_create_piece(AC_ANT, AC_BLACK));

    move = ac_create_move(ac_create_position(4, 4), ac_create_position(4, 6), ac_create_piece(AC_ANTEATER, AC_WHITE));
    ac_add_capture(&move, ac_create_position(3, 5), ac_create_piece(AC_ANT, AC_BLACK));
    ac_add_capture(&move, ac_create_position(3, 6), ac_create_piece(AC_ANT, AC_BLACK));
    ac_add_capture(&move, ac_create_position(4, 6), ac_create_piece(AC_ANT, AC_BLACK));
    ac_add_path_step(&move, ac_create_position(3, 5));
    ac_add_path_step(&move, ac_create_position(3, 6));
    ac_add_path_step(&move, ac_create_position(4, 6));
    ac_set_special_move(&move, AC_ANTEATER_CAPTURE);
    assert(ac_validate_move(&state, move) == 1);
}

/* Verify illegal special moves and mismatched metadata are rejected. */
static void test_validate_move_rejects_illegal_special_moves(void) {
    AcPosition castlingState = create_test_state(AC_WHITE);
    AcPosition enPassantState = create_test_state(AC_WHITE);
    AcMove move;

    ac_set_piece(&castlingState.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&castlingState.board, ac_create_position(7, 9), ac_create_piece(AC_ROOK, AC_WHITE));
    ac_set_piece(&castlingState.board, ac_create_position(0, 5), ac_create_piece(AC_KING, AC_BLACK));
    ac_set_piece(&castlingState.board, ac_create_position(5, 6), ac_create_piece(AC_ROOK, AC_BLACK));
    move = ac_create_move(ac_create_position(7, 5), ac_create_position(7, 7), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_special_move(&move, AC_CASTLING_KINGSIDE);
    assert(ac_validate_move(&castlingState, move) == 0);

    ac_set_piece(&enPassantState.board, ac_create_position(3, 4), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&enPassantState.board, ac_create_position(3, 5), ac_create_piece(AC_ANT, AC_BLACK));
    push_history_move(&enPassantState, ac_create_move(ac_create_position(2, 5), ac_create_position(3, 5),
                                                      ac_create_piece(AC_ANT, AC_BLACK)));
    move = ac_create_move(ac_create_position(3, 4), ac_create_position(2, 5), ac_create_piece(AC_ANT, AC_WHITE));
    ac_add_capture(&move, ac_create_position(2, 5), ac_create_piece(AC_ANT, AC_BLACK));
    ac_set_special_move(&move, AC_EN_PASSANT);
    assert(ac_validate_move(&enPassantState, move) == 0);
}

int main(void) {
    test_validate_selection_reports_expected_result_codes();
    test_validate_move_accepts_basic_ant_advance();
    test_validate_move_rejects_invalid_origin_and_target_inputs();
    test_validate_move_rejects_blocked_rook_path();
    test_validate_move_checks_explicit_capture_metadata();
    test_validate_move_rejects_self_check_positions();
    test_validate_move_accepts_supported_special_moves();
    test_validate_move_accepts_recursive_anteater_capture();
    test_validate_move_rejects_illegal_special_moves();
    return 0;
}
