#include "anteater/rules.hpp"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

using namespace ac;

/*
 * Alignment assumptions for future extensions:
 * - These tests verify move-generation contracts from the public gameplay headers.
 * - Direct king captures are intentionally excluded from generated candidates.
 * - Helpers in this file set up board state only; gameplay legality still belongs to the module under test.
 */

/* Clear the whole board so each test can build a focused position. */
static void clear_board(Board *board) {
    int row;
    int col;

    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            set_piece(board, create_position(row, col), create_piece(PieceType::Empty, Color::Empty));
        }
    }
}

/* Build a minimal Position with an empty board and an explicit side to move. */
static Position create_test_state(Color turn) {
    Position state;

    memset(&state, 0, sizeof(state));
    position_init(&state);
    init_board(&state.board);
    clear_board(&state.board);
    state.castlingRights = 15;
    state.enPassant = create_position(-1, -1);
    state.currentTurn = turn;
    return state;
}

/* Find a generated move by destination square and special-move tag. */
static Move *find_move(MoveList *list, Square to, SpecialMove specialType) {
    int index;

    for (index = 0; index < get_move_count(list); ++index) {
        Move *move = get_move(list, index);

        if (move != NULL && position_equal(move->to, to) && move->specialType == specialType) {
            return move;
        }
    }

    return NULL;
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

/* Check that generation only considers pieces belonging to the side to move. */
static void test_generate_moves_only_for_current_turn(void) {
    Position state = create_test_state(Color::White);
    MoveList list;

    set_piece(&state.board, create_position(6, 4), create_piece(PieceType::Ant, Color::White));
    set_piece(&state.board, create_position(1, 4), create_piece(PieceType::Ant, Color::Black));

    assert(generate_moves(&state, &list) == Status::Ok);
    assert(get_move_count(&list) == 2);
    assert(find_move(&list, create_position(5, 4), SpecialMove::None) != NULL);
    assert(find_move(&list, create_position(4, 4), SpecialMove::None) != NULL);
}

/* Check ant forward movement, opening double-step, and diagonal capture rules. */
static void test_ant_moves_and_capture(void) {
    Position state = create_test_state(Color::White);
    MoveList list;
    Move *captureMove;

    set_piece(&state.board, create_position(6, 4), create_piece(PieceType::Ant, Color::White));
    set_piece(&state.board, create_position(5, 5), create_piece(PieceType::Knight, Color::Black));
    set_piece(&state.board, create_position(5, 3), create_piece(PieceType::Bishop, Color::White));

    assert(generate_legal_moves_for_position(&state, create_position(6, 4), &list) == Status::Ok);
    assert(find_move(&list, create_position(5, 4), SpecialMove::None) != NULL);
    assert(find_move(&list, create_position(4, 4), SpecialMove::None) != NULL);

    captureMove = find_move(&list, create_position(5, 5), SpecialMove::None);
    assert(captureMove != NULL);
    assert(captureMove->captureCount == 1);
    assert(captureMove->captures[0].piece.type == PieceType::Knight);
    assert(find_move(&list, create_position(5, 3), SpecialMove::None) == NULL);
}

/* Check anteater step moves and orthogonal chain-capture generation. */
static void test_anteater_moves_and_chain_capture(void) {
    Position state = create_test_state(Color::White);
    MoveList list;
    Move *stepMove;
    Move *singleCapture;
    Move *chainCapture;

    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Anteater, Color::White));
    set_piece(&state.board, create_position(3, 4), create_piece(PieceType::Ant, Color::Black));
    set_piece(&state.board, create_position(4, 5), create_piece(PieceType::Ant, Color::Black));
    set_piece(&state.board, create_position(4, 6), create_piece(PieceType::Ant, Color::Black));

    assert(generate_legal_moves_for_position(&state, create_position(4, 4), &list) == Status::Ok);

    stepMove = find_move(&list, create_position(3, 3), SpecialMove::None);
    assert(stepMove != NULL);

    singleCapture = find_move(&list, create_position(3, 4), SpecialMove::AnteaterCapture);
    assert(singleCapture != NULL);
    assert(singleCapture->captureCount == 1);
    assert(singleCapture->pathLength == 0);

    chainCapture = find_move(&list, create_position(4, 6), SpecialMove::AnteaterCapture);
    assert(chainCapture != NULL);
    assert(chainCapture->captureCount == 2);
    assert(chainCapture->pathLength == 2);
    assert(position_equal(chainCapture->path[0], create_position(4, 5)) == 1);
    assert(position_equal(chainCapture->path[1], create_position(4, 6)) == 1);
}

/* Check that anteater capture chains may start diagonally, turn orthogonally,
 * and stop on a chosen prefix instead of consuming every reachable branch. */
static void test_anteater_recursive_turning_capture_paths(void) {
    Position state = create_test_state(Color::White);
    MoveList list;
    Move *singleCapture;
    Move *intermediateCapture;
    Move *turnedCapture;
    Move *branchCapture;

    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Anteater, Color::White));
    set_piece(&state.board, create_position(3, 5), create_piece(PieceType::Ant, Color::Black));
    set_piece(&state.board, create_position(3, 6), create_piece(PieceType::Ant, Color::Black));
    set_piece(&state.board, create_position(4, 6), create_piece(PieceType::Ant, Color::Black));
    set_piece(&state.board, create_position(3, 7), create_piece(PieceType::Ant, Color::Black));

    assert(generate_legal_moves_for_position(&state, create_position(4, 4), &list) == Status::Ok);

    singleCapture = find_move(&list, create_position(3, 5), SpecialMove::AnteaterCapture);
    assert(singleCapture != NULL);
    assert(singleCapture->captureCount == 1);
    assert(singleCapture->pathLength == 0);

    intermediateCapture = find_move(&list, create_position(3, 6), SpecialMove::AnteaterCapture);
    assert(intermediateCapture != NULL);
    assert(intermediateCapture->captureCount == 2);
    assert(intermediateCapture->pathLength == 2);
    assert(position_equal(intermediateCapture->path[0], create_position(3, 5)) == 1);
    assert(position_equal(intermediateCapture->path[1], create_position(3, 6)) == 1);

    turnedCapture = find_move(&list, create_position(4, 6), SpecialMove::AnteaterCapture);
    assert(turnedCapture != NULL);
    assert(turnedCapture->captureCount == 3);
    assert(turnedCapture->pathLength == 3);
    assert(position_equal(turnedCapture->path[0], create_position(3, 5)) == 1);
    assert(position_equal(turnedCapture->path[1], create_position(3, 6)) == 1);
    assert(position_equal(turnedCapture->path[2], create_position(4, 6)) == 1);
    assert(position_equal(turnedCapture->captures[2].pos, create_position(4, 6)) == 1);

    branchCapture = find_move(&list, create_position(3, 7), SpecialMove::AnteaterCapture);
    assert(branchCapture != NULL);
    assert(branchCapture->captureCount == 3);
    assert(branchCapture->pathLength == 3);
    assert(position_equal(branchCapture->path[0], create_position(3, 5)) == 1);
    assert(position_equal(branchCapture->path[1], create_position(3, 6)) == 1);
    assert(position_equal(branchCapture->path[2], create_position(3, 7)) == 1);
}

/* Check that sliding pieces stop at the first blocker in each direction. */
static void test_sliding_piece_blocking(void) {
    Position state = create_test_state(Color::White);
    MoveList list;

    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Rook, Color::White));
    set_piece(&state.board, create_position(2, 4), create_piece(PieceType::Ant, Color::Black));
    set_piece(&state.board, create_position(4, 6), create_piece(PieceType::Ant, Color::White));

    assert(generate_legal_moves_for_position(&state, create_position(4, 4), &list) == Status::Ok);
    assert(find_move(&list, create_position(3, 4), SpecialMove::None) != NULL);
    assert(find_move(&list, create_position(2, 4), SpecialMove::None) != NULL);
    assert(find_move(&list, create_position(1, 4), SpecialMove::None) == NULL);
    assert(find_move(&list, create_position(4, 6), SpecialMove::None) == NULL);
}

/* Check combined queen movement and bishop-style diagonal generation. */
static void test_bishop_and_queen_generation(void) {
    Position state = create_test_state(Color::White);
    MoveList list;

    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Queen, Color::White));
    set_piece(&state.board, create_position(6, 6), create_piece(PieceType::Rook, Color::Black));
    set_piece(&state.board, create_position(4, 6), create_piece(PieceType::Bishop, Color::White));

    assert(generate_legal_moves_for_position(&state, create_position(4, 4), &list) == Status::Ok);
    assert(find_move(&list, create_position(4, 5), SpecialMove::None) != NULL);
    assert(find_move(&list, create_position(4, 6), SpecialMove::None) == NULL);
    assert(find_move(&list, create_position(5, 5), SpecialMove::None) != NULL);
    assert(find_move(&list, create_position(6, 6), SpecialMove::None) != NULL);
}

/* Check knight jump captures and king single-step landing rules. */
static void test_knight_and_king_moves(void) {
    Position state = create_test_state(Color::White);
    MoveList knightList;
    MoveList kingList;
    Move *captureMove;

    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Knight, Color::White));
    set_piece(&state.board, create_position(4, 5), create_piece(PieceType::Ant, Color::White));
    set_piece(&state.board, create_position(3, 4), create_piece(PieceType::Ant, Color::White));
    set_piece(&state.board, create_position(2, 5), create_piece(PieceType::Bishop, Color::Black));

    assert(generate_legal_moves_for_position(&state, create_position(4, 4), &knightList) == Status::Ok);
    captureMove = find_move(&knightList, create_position(2, 5), SpecialMove::None);
    assert(captureMove != NULL);
    assert(captureMove->captureCount == 1);

    clear_board(&state.board);
    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::King, Color::White));
    set_piece(&state.board, create_position(4, 5), create_piece(PieceType::Ant, Color::White));
    set_piece(&state.board, create_position(5, 5), create_piece(PieceType::Ant, Color::Black));

    assert(generate_legal_moves_for_position(&state, create_position(4, 4), &kingList) == Status::Ok);
    assert(find_move(&kingList, create_position(4, 5), SpecialMove::None) == NULL);
    assert(find_move(&kingList, create_position(5, 5), SpecialMove::None) != NULL);
}

/* Check that promotion candidates are generated as four variants for forward
 * and capture cases, for both colors. */
static void test_promotion_generation(void) {
    Position whiteState = create_test_state(Color::White);
    Position blackState = create_test_state(Color::Black);
    MoveList list;
    Move *move;

    set_piece(&whiteState.board, create_position(1, 4), create_piece(PieceType::Ant, Color::White));
    set_piece(&whiteState.board, create_position(0, 5), create_piece(PieceType::Rook, Color::Black));

    assert(generate_legal_moves_for_position(&whiteState, create_position(1, 4), &list) == Status::Ok);
    assert(find_move(&list, create_position(0, 4), SpecialMove::None) == NULL);
    assert(find_move(&list, create_position(0, 4), SpecialMove::PromotionQueen) != NULL);
    assert(find_move(&list, create_position(0, 4), SpecialMove::PromotionRook) != NULL);
    assert(find_move(&list, create_position(0, 4), SpecialMove::PromotionBishop) != NULL);
    assert(find_move(&list, create_position(0, 4), SpecialMove::PromotionKnight) != NULL);

    move = find_move(&list, create_position(0, 5), SpecialMove::PromotionQueen);
    assert(move != NULL);
    assert(move->captureCount == 1);
    assert(move->captures[0].piece.type == PieceType::Rook);

    set_piece(&blackState.board, create_position(6, 4), create_piece(PieceType::Ant, Color::Black));
    assert(generate_legal_moves_for_position(&blackState, create_position(6, 4), &list) == Status::Ok);
    assert(find_move(&list, create_position(7, 4), SpecialMove::PromotionQueen) != NULL);
    assert(find_move(&list, create_position(7, 4), SpecialMove::PromotionRook) != NULL);
    assert(find_move(&list, create_position(7, 4), SpecialMove::PromotionBishop) != NULL);
    assert(find_move(&list, create_position(7, 4), SpecialMove::PromotionKnight) != NULL);
}

/* Check castling generation, plus the main blocking and attack-based rejection
 * cases reconstructed from board state and history. */
static void test_castling_generation(void) {
    Position state = create_test_state(Color::White);
    MoveList list;

    set_piece(&state.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&state.board, create_position(7, 0), create_piece(PieceType::Rook, Color::White));
    set_piece(&state.board, create_position(7, 9), create_piece(PieceType::Rook, Color::White));
    set_piece(&state.board, create_position(0, 5), create_piece(PieceType::King, Color::Black));

    assert(generate_legal_moves_for_position(&state, create_position(7, 5), &list) == Status::Ok);
    assert(find_move(&list, create_position(7, 7), SpecialMove::CastlingKingside) != NULL);
    assert(find_move(&list, create_position(7, 3), SpecialMove::CastlingQueenside) != NULL);

    set_piece(&state.board, create_position(7, 6), create_piece(PieceType::Bishop, Color::White));
    assert(generate_legal_moves_for_position(&state, create_position(7, 5), &list) == Status::Ok);
    assert(find_move(&list, create_position(7, 7), SpecialMove::CastlingKingside) == NULL);

    clear_board(&state.board);
    state.castlingRights = 15;
    state.enPassant = create_position(-1, -1);
    state.moveCount = 0;
    set_piece(&state.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&state.board, create_position(7, 0), create_piece(PieceType::Rook, Color::White));
    set_piece(&state.board, create_position(7, 9), create_piece(PieceType::Rook, Color::White));
    set_piece(&state.board, create_position(0, 5), create_piece(PieceType::King, Color::Black));
    set_piece(&state.board, create_position(5, 6), create_piece(PieceType::Rook, Color::Black));
    assert(generate_legal_moves_for_position(&state, create_position(7, 5), &list) == Status::Ok);
    assert(find_move(&list, create_position(7, 7), SpecialMove::CastlingKingside) == NULL);

    clear_board(&state.board);
    state.castlingRights = 15;
    state.enPassant = create_position(-1, -1);
    state.moveCount = 0;
    set_piece(&state.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&state.board, create_position(7, 0), create_piece(PieceType::Rook, Color::White));
    set_piece(&state.board, create_position(7, 9), create_piece(PieceType::Rook, Color::White));
    set_piece(&state.board, create_position(0, 5), create_piece(PieceType::King, Color::Black));
    push_history_move(
        &state, create_move(create_position(7, 9), create_position(7, 8), create_piece(PieceType::Rook, Color::White)));
    assert(generate_legal_moves_for_position(&state, create_position(7, 5), &list) == Status::Ok);
    assert(find_move(&list, create_position(7, 7), SpecialMove::CastlingKingside) == NULL);
}

/* Check en passant generation from the latest double-step ant move only. */
static void test_en_passant_generation(void) {
    Position state = create_test_state(Color::White);
    MoveList list;
    Move lastMove;
    Move *epMove;

    set_piece(&state.board, create_position(3, 4), create_piece(PieceType::Ant, Color::White));
    set_piece(&state.board, create_position(3, 5), create_piece(PieceType::Ant, Color::Black));

    lastMove = create_move(create_position(1, 5), create_position(3, 5), create_piece(PieceType::Ant, Color::Black));
    push_history_move(&state, lastMove);

    assert(generate_legal_moves_for_position(&state, create_position(3, 4), &list) == Status::Ok);
    epMove = find_move(&list, create_position(2, 5), SpecialMove::EnPassant);
    assert(epMove != NULL);
    assert(epMove->captureCount == 1);
    assert(position_equal(epMove->captures[0].pos, create_position(3, 5)) == 1);

    state.castlingRights = 15;
    state.enPassant = create_position(-1, -1);
    state.moveCount = 0;
    lastMove = create_move(create_position(2, 5), create_position(3, 5), create_piece(PieceType::Ant, Color::Black));
    push_history_move(&state, lastMove);
    assert(generate_legal_moves_for_position(&state, create_position(3, 4), &list) == Status::Ok);
    assert(find_move(&list, create_position(2, 5), SpecialMove::EnPassant) == NULL);
}

/* Check that no piece generator emits a direct capture onto an enemy king square. */
static void test_movegen_does_not_generate_king_captures(void) {
    Position state = create_test_state(Color::White);
    MoveList list;

    set_piece(&state.board, create_position(6, 4), create_piece(PieceType::Ant, Color::White));
    set_piece(&state.board, create_position(5, 5), create_piece(PieceType::King, Color::Black));
    assert(generate_legal_moves_for_position(&state, create_position(6, 4), &list) == Status::Ok);
    assert(find_move(&list, create_position(5, 5), SpecialMove::None) == NULL);

    clear_board(&state.board);
    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Anteater, Color::White));
    set_piece(&state.board, create_position(3, 4), create_piece(PieceType::King, Color::Black));
    assert(generate_legal_moves_for_position(&state, create_position(4, 4), &list) == Status::Ok);
    assert(find_move(&list, create_position(3, 4), SpecialMove::AnteaterCapture) == NULL);

    clear_board(&state.board);
    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Rook, Color::White));
    set_piece(&state.board, create_position(4, 7), create_piece(PieceType::King, Color::Black));
    assert(generate_legal_moves_for_position(&state, create_position(4, 4), &list) == Status::Ok);
    assert(find_move(&list, create_position(4, 7), SpecialMove::None) == NULL);

    clear_board(&state.board);
    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Queen, Color::White));
    set_piece(&state.board, create_position(4, 7), create_piece(PieceType::King, Color::Black));
    assert(generate_legal_moves_for_position(&state, create_position(4, 4), &list) == Status::Ok);
    assert(find_move(&list, create_position(4, 7), SpecialMove::None) == NULL);

    clear_board(&state.board);
    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::Knight, Color::White));
    set_piece(&state.board, create_position(2, 5), create_piece(PieceType::King, Color::Black));
    assert(generate_legal_moves_for_position(&state, create_position(4, 4), &list) == Status::Ok);
    assert(find_move(&list, create_position(2, 5), SpecialMove::None) == NULL);

    clear_board(&state.board);
    set_piece(&state.board, create_position(4, 4), create_piece(PieceType::King, Color::White));
    set_piece(&state.board, create_position(5, 5), create_piece(PieceType::King, Color::Black));
    assert(generate_legal_moves_for_position(&state, create_position(4, 4), &list) == Status::Ok);
    assert(find_move(&list, create_position(5, 5), SpecialMove::None) == NULL);
}

/* Check edge-board counts and integration with selection/move validation helpers. */
static void test_edge_counts_and_validation(void) {
    Position state = create_test_state(Color::White);
    MoveList list;
    Move legalMove;
    Move invalidMove;

    set_piece(&state.board, create_position(0, 0), create_piece(PieceType::Rook, Color::White));
    assert(generate_legal_moves_for_position(&state, create_position(0, 0), &list) == Status::Ok);
    assert(get_move_count(&list) == 16);

    clear_board(&state.board);
    set_piece(&state.board, create_position(6, 4), create_piece(PieceType::Ant, Color::White));
    set_piece(&state.board, create_position(5, 5), create_piece(PieceType::Rook, Color::Black));

    assert(validate_selection(&state, create_position(-1, 0)) == SelectionResult::OutOfBounds);
    assert(validate_selection(&state, create_position(0, 0)) == SelectionResult::Empty);
    assert(validate_selection(&state, create_position(6, 4)) == SelectionResult::Valid);
    assert(validate_selection(&state, create_position(5, 5)) == SelectionResult::OpponentPiece);

    legalMove = create_move(create_position(6, 4), create_position(5, 4), create_piece(PieceType::Ant, Color::White));
    invalidMove =
        create_move(create_position(6, 4), create_position(6, 10), create_piece(PieceType::Ant, Color::White));
    assert(validate_move(&state, legalMove) == 1);
    assert(validate_move(&state, invalidMove) == 0);
}

/* Run the move-generation regression suite for the supported piece rules. */
int main(void) {
    test_generate_moves_only_for_current_turn();
    test_ant_moves_and_capture();
    test_anteater_moves_and_chain_capture();
    test_anteater_recursive_turning_capture_paths();
    test_sliding_piece_blocking();
    test_bishop_and_queen_generation();
    test_knight_and_king_moves();
    test_promotion_generation();
    test_castling_generation();
    test_en_passant_generation();
    test_movegen_does_not_generate_king_captures();
    test_edge_counts_and_validation();
    return 0;
}
