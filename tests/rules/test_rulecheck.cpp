#include "anteater/rules.hpp"
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>

using namespace ac;

/* Clear the whole board so each test can build one focused validation case. */
static void clear_board(Board *board) {
    int row;
    int col;

    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            set_piece(board, create_position(row, col), create_piece(PieceType::Empty, Color::Empty));
        }
    }
}

/* Build a minimal game state for standalone rule-checking tests. */
static Position create_test_state(Color turn) {
    GameConfig config;
    Position state;

    init_default_game_config(&config);
    position_init(&state);
    clear_board(&state.board);
    state.currentTurn = turn;
    return state;
}

/* Record one historical move without mutating the current board state. */
static void push_history_move(Position *state, Move move) {
    state->enPassant = move.movedPiece.type == PieceType::Ant && abs(move.to.row - move.from.row) == 2
                           ? move.to
                           : create_position(-1, -1);
    if (move.movedPiece.type == PieceType::King)
        state->castlingRights &= ~(3 << (move.movedPiece.color == Color::White ? 0 : 2));
    if (move.movedPiece.type == PieceType::Rook && move.from.col == 9)
        state->castlingRights &= ~(1 << (move.movedPiece.color == Color::White ? 0 : 2));
    if (move.movedPiece.type == PieceType::Rook && move.from.col == 0)
        state->castlingRights &= ~(2 << (move.movedPiece.color == Color::White ? 0 : 2));
    ++state->moveCount;
}

/* Verify selection results differentiate empty, opponent, and invalid squares. */
static void test_validate_selection_reports_expected_result_codes(void) {
    Position state = create_test_state(Color::White);

    set_piece(&state.board, create_position(6, 4), create_piece(PieceType::Ant, Color::White));
    set_piece(&state.board, create_position(1, 4), create_piece(PieceType::Rook, Color::Black));

    assert(validate_selection(&state, create_position(-1, 0)) == SelectionResult::OutOfBounds);
    assert(validate_selection(&state, create_position(4, 4)) == SelectionResult::Empty);
    assert(validate_selection(&state, create_position(6, 4)) == SelectionResult::Valid);
    assert(validate_selection(&state, create_position(1, 4)) == SelectionResult::OpponentPiece);
}

/* Verify the rule checker accepts one ordinary legal move request. */
static void test_validate_move_accepts_basic_ant_advance(void) {
    Position state = create_test_state(Color::White);
    Move move;

    set_piece(&state.board, create_position(6, 4), create_piece(PieceType::Ant, Color::White));

    move = create_move(create_position(6, 4), create_position(5, 4), create_piece(PieceType::Ant, Color::White));
    assert(validate_move(&state, move) == 1);
}

/* Verify off-board targets and non-playable origins are rejected cleanly. */
static void test_validate_move_rejects_invalid_origin_and_target_inputs(void) {
    Position state = create_test_state(Color::White);
    Move emptySquareMove;
    Move opponentPieceMove;
    Move outOfBoundsMove;

    set_piece(&state.board, create_position(6, 4), create_piece(PieceType::Ant, Color::White));
    set_piece(&state.board, create_position(1, 4), create_piece(PieceType::Ant, Color::Black));

    emptySquareMove =
        create_move(create_position(4, 4), create_position(3, 4), create_piece(PieceType::Ant, Color::White));
    opponentPieceMove =
        create_move(create_position(1, 4), create_position(2, 4), create_piece(PieceType::Ant, Color::Black));
    outOfBoundsMove =
        create_move(create_position(6, 4), create_position(6, 10), create_piece(PieceType::Ant, Color::White));

    assert(validate_move(&state, emptySquareMove) == 0);
    assert(validate_move(&state, opponentPieceMove) == 0);
    assert(validate_move(&state, outOfBoundsMove) == 0);
}

/* Verify blocked sliding moves are not treated as legal chess moves. */
static void test_validate_move_rejects_blocked_rook_path(void) {
    Position state = create_test_state(Color::White);
    Move blockedMove;

    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Rook, Color::White));
    set_piece(&state.board, create_position(4, 6), create_piece(PieceType::Ant, Color::White));

    blockedMove =
        create_move(create_position(4, 4), create_position(4, 7), create_piece(PieceType::Rook, Color::White));
    assert(validate_move(&state, blockedMove) == 0);
}

/* Verify explicit capture metadata must match the generated legal move exactly. */
static void test_validate_move_checks_explicit_capture_metadata(void) {
    Position state = create_test_state(Color::White);
    Move partialRequest;
    Move exactCaptureRequest;
    Move wrongCaptureRequest;

    set_piece(&state.board, create_position(6, 4), create_piece(PieceType::Ant, Color::White));
    set_piece(&state.board, create_position(5, 5), create_piece(PieceType::Knight, Color::Black));

    partialRequest =
        create_move(create_position(6, 4), create_position(5, 5), create_piece(PieceType::Ant, Color::White));

    exactCaptureRequest =
        create_move(create_position(6, 4), create_position(5, 5), create_piece(PieceType::Ant, Color::White));
    add_capture(&exactCaptureRequest, create_position(5, 5), create_piece(PieceType::Knight, Color::Black));

    wrongCaptureRequest =
        create_move(create_position(6, 4), create_position(5, 5), create_piece(PieceType::Ant, Color::White));
    add_capture(&wrongCaptureRequest, create_position(5, 5), create_piece(PieceType::Bishop, Color::Black));

    assert(validate_move(&state, partialRequest) == 1);
    assert(validate_move(&state, exactCaptureRequest) == 1);
    assert(validate_move(&state, wrongCaptureRequest) == 0);
}

/* Verify legal filtering rejects moves that would expose the moving side's
 * king or move the king onto an attacked square. */
static void test_validate_move_rejects_self_check_positions(void) {
    Position pinnedState = create_test_state(Color::White);
    Position kingStepState = create_test_state(Color::White);
    Move move;

    set_piece(&pinnedState.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&pinnedState.board, create_position(7, 4), create_piece(PieceType::Rook, Color::White));
    set_piece(&pinnedState.board, create_position(7, 0), create_piece(PieceType::Rook, Color::Black));
    set_piece(&pinnedState.board, create_position(0, 5), create_piece(PieceType::King, Color::Black));
    move = create_move(create_position(7, 4), create_position(6, 4), create_piece(PieceType::Rook, Color::White));
    assert(validate_move(&pinnedState, move) == 0);

    set_piece(&kingStepState.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&kingStepState.board, create_position(0, 0), create_piece(PieceType::King, Color::Black));
    set_piece(&kingStepState.board, create_position(5, 4), create_piece(PieceType::Rook, Color::Black));
    move = create_move(create_position(7, 5), create_position(6, 4), create_piece(PieceType::King, Color::White));
    assert(validate_move(&kingStepState, move) == 0);
}

/* Verify special move validation accepts castling, en passant, and both
 * explicit and implicit promotion requests. */
static void test_validate_move_accepts_supported_special_moves(void) {
    Position promotionState = create_test_state(Color::White);
    Position castlingState = create_test_state(Color::White);
    Position enPassantState = create_test_state(Color::White);
    Move move;

    set_piece(&promotionState.board, create_position(1, 2), create_piece(PieceType::Ant, Color::White));
    move = create_move(create_position(1, 2), create_position(0, 2), create_piece(PieceType::Ant, Color::White));
    assert(validate_move(&promotionState, move) == 1);
    set_special_move(&move, SpecialMove::PromotionRook);
    assert(validate_move(&promotionState, move) == 1);

    set_piece(&castlingState.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&castlingState.board, create_position(7, 9), create_piece(PieceType::Rook, Color::White));
    set_piece(&castlingState.board, create_position(0, 5), create_piece(PieceType::King, Color::Black));
    move = create_move(create_position(7, 5), create_position(7, 7), create_piece(PieceType::King, Color::White));
    set_special_move(&move, SpecialMove::CastlingKingside);
    assert(validate_move(&castlingState, move) == 1);

    set_piece(&enPassantState.board, create_position(3, 4), create_piece(PieceType::Ant, Color::White));
    set_piece(&enPassantState.board, create_position(3, 5), create_piece(PieceType::Ant, Color::Black));
    push_history_move(&enPassantState, create_move(create_position(1, 5), create_position(3, 5),
                                                   create_piece(PieceType::Ant, Color::Black)));
    move = create_move(create_position(3, 4), create_position(2, 5), create_piece(PieceType::Ant, Color::White));
    add_capture(&move, create_position(3, 5), create_piece(PieceType::Ant, Color::Black));
    set_special_move(&move, SpecialMove::EnPassant);
    assert(validate_move(&enPassantState, move) == 1);
}

/* Verify explicit anteater recursion metadata is accepted when it matches a
 * generated legal turning chain exactly. */
static void test_validate_move_accepts_recursive_anteater_capture(void) {
    Position state = create_test_state(Color::White);
    Move move;

    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Anteater, Color::White));
    set_piece(&state.board, create_position(3, 5), create_piece(PieceType::Ant, Color::Black));
    set_piece(&state.board, create_position(3, 6), create_piece(PieceType::Ant, Color::Black));
    set_piece(&state.board, create_position(4, 6), create_piece(PieceType::Ant, Color::Black));
    set_piece(&state.board, create_position(3, 7), create_piece(PieceType::Ant, Color::Black));

    move = create_move(create_position(4, 4), create_position(4, 6), create_piece(PieceType::Anteater, Color::White));
    add_capture(&move, create_position(3, 5), create_piece(PieceType::Ant, Color::Black));
    add_capture(&move, create_position(3, 6), create_piece(PieceType::Ant, Color::Black));
    add_capture(&move, create_position(4, 6), create_piece(PieceType::Ant, Color::Black));
    add_path_step(&move, create_position(3, 5));
    add_path_step(&move, create_position(3, 6));
    add_path_step(&move, create_position(4, 6));
    set_special_move(&move, SpecialMove::AnteaterCapture);
    assert(validate_move(&state, move) == 1);
}

/* Verify illegal special moves and mismatched metadata are rejected. */
static void test_validate_move_rejects_illegal_special_moves(void) {
    Position castlingState = create_test_state(Color::White);
    Position enPassantState = create_test_state(Color::White);
    Move move;

    set_piece(&castlingState.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&castlingState.board, create_position(7, 9), create_piece(PieceType::Rook, Color::White));
    set_piece(&castlingState.board, create_position(0, 5), create_piece(PieceType::King, Color::Black));
    set_piece(&castlingState.board, create_position(5, 6), create_piece(PieceType::Rook, Color::Black));
    move = create_move(create_position(7, 5), create_position(7, 7), create_piece(PieceType::King, Color::White));
    set_special_move(&move, SpecialMove::CastlingKingside);
    assert(validate_move(&castlingState, move) == 0);

    set_piece(&enPassantState.board, create_position(3, 4), create_piece(PieceType::Ant, Color::White));
    set_piece(&enPassantState.board, create_position(3, 5), create_piece(PieceType::Ant, Color::Black));
    push_history_move(&enPassantState, create_move(create_position(2, 5), create_position(3, 5),
                                                   create_piece(PieceType::Ant, Color::Black)));
    move = create_move(create_position(3, 4), create_position(2, 5), create_piece(PieceType::Ant, Color::White));
    add_capture(&move, create_position(2, 5), create_piece(PieceType::Ant, Color::Black));
    set_special_move(&move, SpecialMove::EnPassant);
    assert(validate_move(&enPassantState, move) == 0);
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
