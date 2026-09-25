#include "anteater/rules.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/*
 * Alignment assumptions for future extensions:
 * - These tests verify move-generation contracts from the public gameplay headers.
 * - Direct king captures are intentionally excluded from generated candidates.
 * - Helpers in this file set up board state only; gameplay legality still belongs to the module under test.
 */

/* Clear the whole board so each test can build a focused position. */
static void clear_board(AcBoard *board) {
    int row;
    int col;

    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            ac_set_piece(board, ac_create_position(row, col), ac_create_piece(AC_EMPTY_PIECE, AC_EMPTY_COLOR));
        }
    }
}

/* Build a minimal AcPosition with an empty board and an explicit side to move. */
static AcPosition create_test_state(AcColor turn) {
    AcPosition state;

    memset(&state, 0, sizeof(state));
    ac_position_init(&state);
    ac_init_board(&state.board);
    clear_board(&state.board);
    state.castlingRights = 15;
    state.enPassant = ac_create_position(-1, -1);
    state.currentTurn = turn;
    return state;
}

/* Find a generated move by destination square and special-move tag. */
static AcMove *find_move(AcMoveList *list, AcSquare to, AcSpecialMove specialType) {
    int index;

    for (index = 0; index < ac_get_move_count(list); ++index) {
        AcMove *move = ac_get_move(list, index);

        if (move != NULL && ac_position_equal(move->to, to) && move->specialType == specialType) {
            return move;
        }
    }

    return NULL;
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

/* Check that generation only considers pieces belonging to the side to move. */
static void test_generate_moves_only_for_current_turn(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMoveList list;

    ac_set_piece(&state.board, ac_create_position(6, 4), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(1, 4), ac_create_piece(AC_ANT, AC_BLACK));

    assert(ac_generate_moves(&state, &list) == 0);
    assert(ac_get_move_count(&list) == 2);
    assert(find_move(&list, ac_create_position(5, 4), AC_NO_SPECIAL_MOVE) != NULL);
    assert(find_move(&list, ac_create_position(4, 4), AC_NO_SPECIAL_MOVE) != NULL);
}

/* Check ant forward movement, opening double-step, and diagonal capture rules. */
static void test_ant_moves_and_capture(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMoveList list;
    AcMove *captureMove;

    ac_set_piece(&state.board, ac_create_position(6, 4), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(5, 5), ac_create_piece(AC_KNIGHT, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(5, 3), ac_create_piece(AC_BISHOP, AC_WHITE));

    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(6, 4), &list) == 0);
    assert(find_move(&list, ac_create_position(5, 4), AC_NO_SPECIAL_MOVE) != NULL);
    assert(find_move(&list, ac_create_position(4, 4), AC_NO_SPECIAL_MOVE) != NULL);

    captureMove = find_move(&list, ac_create_position(5, 5), AC_NO_SPECIAL_MOVE);
    assert(captureMove != NULL);
    assert(captureMove->captureCount == 1);
    assert(captureMove->captures[0].piece.type == AC_KNIGHT);
    assert(find_move(&list, ac_create_position(5, 3), AC_NO_SPECIAL_MOVE) == NULL);
}

/* Check anteater step moves and orthogonal chain-capture generation. */
static void test_anteater_moves_and_chain_capture(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMoveList list;
    AcMove *stepMove;
    AcMove *singleCapture;
    AcMove *chainCapture;

    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_ANTEATER, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(3, 4), ac_create_piece(AC_ANT, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(4, 5), ac_create_piece(AC_ANT, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(4, 6), ac_create_piece(AC_ANT, AC_BLACK));

    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(4, 4), &list) == 0);

    stepMove = find_move(&list, ac_create_position(3, 3), AC_NO_SPECIAL_MOVE);
    assert(stepMove != NULL);

    singleCapture = find_move(&list, ac_create_position(3, 4), AC_ANTEATER_CAPTURE);
    assert(singleCapture != NULL);
    assert(singleCapture->captureCount == 1);
    assert(singleCapture->pathLength == 0);

    chainCapture = find_move(&list, ac_create_position(4, 6), AC_ANTEATER_CAPTURE);
    assert(chainCapture != NULL);
    assert(chainCapture->captureCount == 2);
    assert(chainCapture->pathLength == 2);
    assert(ac_position_equal(chainCapture->path[0], ac_create_position(4, 5)) == 1);
    assert(ac_position_equal(chainCapture->path[1], ac_create_position(4, 6)) == 1);
}

/* Check that anteater capture chains may start diagonally, turn orthogonally,
 * and stop on a chosen prefix instead of consuming every reachable branch. */
static void test_anteater_recursive_turning_capture_paths(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMoveList list;
    AcMove *singleCapture;
    AcMove *intermediateCapture;
    AcMove *turnedCapture;
    AcMove *branchCapture;

    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_ANTEATER, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(3, 5), ac_create_piece(AC_ANT, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(3, 6), ac_create_piece(AC_ANT, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(4, 6), ac_create_piece(AC_ANT, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(3, 7), ac_create_piece(AC_ANT, AC_BLACK));

    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(4, 4), &list) == 0);

    singleCapture = find_move(&list, ac_create_position(3, 5), AC_ANTEATER_CAPTURE);
    assert(singleCapture != NULL);
    assert(singleCapture->captureCount == 1);
    assert(singleCapture->pathLength == 0);

    intermediateCapture = find_move(&list, ac_create_position(3, 6), AC_ANTEATER_CAPTURE);
    assert(intermediateCapture != NULL);
    assert(intermediateCapture->captureCount == 2);
    assert(intermediateCapture->pathLength == 2);
    assert(ac_position_equal(intermediateCapture->path[0], ac_create_position(3, 5)) == 1);
    assert(ac_position_equal(intermediateCapture->path[1], ac_create_position(3, 6)) == 1);

    turnedCapture = find_move(&list, ac_create_position(4, 6), AC_ANTEATER_CAPTURE);
    assert(turnedCapture != NULL);
    assert(turnedCapture->captureCount == 3);
    assert(turnedCapture->pathLength == 3);
    assert(ac_position_equal(turnedCapture->path[0], ac_create_position(3, 5)) == 1);
    assert(ac_position_equal(turnedCapture->path[1], ac_create_position(3, 6)) == 1);
    assert(ac_position_equal(turnedCapture->path[2], ac_create_position(4, 6)) == 1);
    assert(ac_position_equal(turnedCapture->captures[2].pos, ac_create_position(4, 6)) == 1);

    branchCapture = find_move(&list, ac_create_position(3, 7), AC_ANTEATER_CAPTURE);
    assert(branchCapture != NULL);
    assert(branchCapture->captureCount == 3);
    assert(branchCapture->pathLength == 3);
    assert(ac_position_equal(branchCapture->path[0], ac_create_position(3, 5)) == 1);
    assert(ac_position_equal(branchCapture->path[1], ac_create_position(3, 6)) == 1);
    assert(ac_position_equal(branchCapture->path[2], ac_create_position(3, 7)) == 1);
}

/* Check that sliding pieces stop at the first blocker in each direction. */
static void test_sliding_piece_blocking(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMoveList list;

    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_ROOK, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(2, 4), ac_create_piece(AC_ANT, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(4, 6), ac_create_piece(AC_ANT, AC_WHITE));

    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(4, 4), &list) == 0);
    assert(find_move(&list, ac_create_position(3, 4), AC_NO_SPECIAL_MOVE) != NULL);
    assert(find_move(&list, ac_create_position(2, 4), AC_NO_SPECIAL_MOVE) != NULL);
    assert(find_move(&list, ac_create_position(1, 4), AC_NO_SPECIAL_MOVE) == NULL);
    assert(find_move(&list, ac_create_position(4, 6), AC_NO_SPECIAL_MOVE) == NULL);
}

/* Check combined queen movement and bishop-style diagonal generation. */
static void test_bishop_and_queen_generation(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMoveList list;

    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_QUEEN, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(6, 6), ac_create_piece(AC_ROOK, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(4, 6), ac_create_piece(AC_BISHOP, AC_WHITE));

    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(4, 4), &list) == 0);
    assert(find_move(&list, ac_create_position(4, 5), AC_NO_SPECIAL_MOVE) != NULL);
    assert(find_move(&list, ac_create_position(4, 6), AC_NO_SPECIAL_MOVE) == NULL);
    assert(find_move(&list, ac_create_position(5, 5), AC_NO_SPECIAL_MOVE) != NULL);
    assert(find_move(&list, ac_create_position(6, 6), AC_NO_SPECIAL_MOVE) != NULL);
}

/* Check knight jump captures and king single-step landing rules. */
static void test_knight_and_king_moves(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMoveList knightList;
    AcMoveList kingList;
    AcMove *captureMove;

    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_KNIGHT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(4, 5), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(3, 4), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(2, 5), ac_create_piece(AC_BISHOP, AC_BLACK));

    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(4, 4), &knightList) == 0);
    captureMove = find_move(&knightList, ac_create_position(2, 5), AC_NO_SPECIAL_MOVE);
    assert(captureMove != NULL);
    assert(captureMove->captureCount == 1);

    clear_board(&state.board);
    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(4, 5), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(5, 5), ac_create_piece(AC_ANT, AC_BLACK));

    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(4, 4), &kingList) == 0);
    assert(find_move(&kingList, ac_create_position(4, 5), AC_NO_SPECIAL_MOVE) == NULL);
    assert(find_move(&kingList, ac_create_position(5, 5), AC_NO_SPECIAL_MOVE) != NULL);
}

/* Check that promotion candidates are generated as four variants for forward
 * and capture cases, for both colors. */
static void test_promotion_generation(void) {
    AcPosition whiteState = create_test_state(AC_WHITE);
    AcPosition blackState = create_test_state(AC_BLACK);
    AcMoveList list;
    AcMove *move;

    ac_set_piece(&whiteState.board, ac_create_position(1, 4), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&whiteState.board, ac_create_position(0, 5), ac_create_piece(AC_ROOK, AC_BLACK));

    assert(ac_generate_legal_moves_for_position(&whiteState, ac_create_position(1, 4), &list) == 0);
    assert(find_move(&list, ac_create_position(0, 4), AC_NO_SPECIAL_MOVE) == NULL);
    assert(find_move(&list, ac_create_position(0, 4), AC_PROMOTION_QUEEN) != NULL);
    assert(find_move(&list, ac_create_position(0, 4), AC_PROMOTION_ROOK) != NULL);
    assert(find_move(&list, ac_create_position(0, 4), AC_PROMOTION_BISHOP) != NULL);
    assert(find_move(&list, ac_create_position(0, 4), AC_PROMOTION_KNIGHT) != NULL);

    move = find_move(&list, ac_create_position(0, 5), AC_PROMOTION_QUEEN);
    assert(move != NULL);
    assert(move->captureCount == 1);
    assert(move->captures[0].piece.type == AC_ROOK);

    ac_set_piece(&blackState.board, ac_create_position(6, 4), ac_create_piece(AC_ANT, AC_BLACK));
    assert(ac_generate_legal_moves_for_position(&blackState, ac_create_position(6, 4), &list) == 0);
    assert(find_move(&list, ac_create_position(7, 4), AC_PROMOTION_QUEEN) != NULL);
    assert(find_move(&list, ac_create_position(7, 4), AC_PROMOTION_ROOK) != NULL);
    assert(find_move(&list, ac_create_position(7, 4), AC_PROMOTION_BISHOP) != NULL);
    assert(find_move(&list, ac_create_position(7, 4), AC_PROMOTION_KNIGHT) != NULL);
}

/* Check castling generation, plus the main blocking and attack-based rejection
 * cases reconstructed from board state and history. */
static void test_castling_generation(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMoveList list;

    ac_set_piece(&state.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(7, 0), ac_create_piece(AC_ROOK, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(7, 9), ac_create_piece(AC_ROOK, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(0, 5), ac_create_piece(AC_KING, AC_BLACK));

    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(7, 5), &list) == 0);
    assert(find_move(&list, ac_create_position(7, 7), AC_CASTLING_KINGSIDE) != NULL);
    assert(find_move(&list, ac_create_position(7, 3), AC_CASTLING_QUEENSIDE) != NULL);

    ac_set_piece(&state.board, ac_create_position(7, 6), ac_create_piece(AC_BISHOP, AC_WHITE));
    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(7, 5), &list) == 0);
    assert(find_move(&list, ac_create_position(7, 7), AC_CASTLING_KINGSIDE) == NULL);

    clear_board(&state.board);
    state.castlingRights = 15;
    state.enPassant = ac_create_position(-1, -1);
    state.moveCount = 0;
    ac_set_piece(&state.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(7, 0), ac_create_piece(AC_ROOK, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(7, 9), ac_create_piece(AC_ROOK, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(0, 5), ac_create_piece(AC_KING, AC_BLACK));
    ac_set_piece(&state.board, ac_create_position(5, 6), ac_create_piece(AC_ROOK, AC_BLACK));
    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(7, 5), &list) == 0);
    assert(find_move(&list, ac_create_position(7, 7), AC_CASTLING_KINGSIDE) == NULL);

    clear_board(&state.board);
    state.castlingRights = 15;
    state.enPassant = ac_create_position(-1, -1);
    state.moveCount = 0;
    ac_set_piece(&state.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(7, 0), ac_create_piece(AC_ROOK, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(7, 9), ac_create_piece(AC_ROOK, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(0, 5), ac_create_piece(AC_KING, AC_BLACK));
    push_history_move(
        &state, ac_create_move(ac_create_position(7, 9), ac_create_position(7, 8), ac_create_piece(AC_ROOK, AC_WHITE)));
    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(7, 5), &list) == 0);
    assert(find_move(&list, ac_create_position(7, 7), AC_CASTLING_KINGSIDE) == NULL);
}

/* Check en passant generation from the latest double-step ant move only. */
static void test_en_passant_generation(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMoveList list;
    AcMove lastMove;
    AcMove *epMove;

    ac_set_piece(&state.board, ac_create_position(3, 4), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(3, 5), ac_create_piece(AC_ANT, AC_BLACK));

    lastMove = ac_create_move(ac_create_position(1, 5), ac_create_position(3, 5), ac_create_piece(AC_ANT, AC_BLACK));
    push_history_move(&state, lastMove);

    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(3, 4), &list) == 0);
    epMove = find_move(&list, ac_create_position(2, 5), AC_EN_PASSANT);
    assert(epMove != NULL);
    assert(epMove->captureCount == 1);
    assert(ac_position_equal(epMove->captures[0].pos, ac_create_position(3, 5)) == 1);

    state.castlingRights = 15;
    state.enPassant = ac_create_position(-1, -1);
    state.moveCount = 0;
    lastMove = ac_create_move(ac_create_position(2, 5), ac_create_position(3, 5), ac_create_piece(AC_ANT, AC_BLACK));
    push_history_move(&state, lastMove);
    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(3, 4), &list) == 0);
    assert(find_move(&list, ac_create_position(2, 5), AC_EN_PASSANT) == NULL);
}

/* Check that no piece generator emits a direct capture onto an enemy king square. */
static void test_movegen_does_not_generate_king_captures(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMoveList list;

    ac_set_piece(&state.board, ac_create_position(6, 4), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(5, 5), ac_create_piece(AC_KING, AC_BLACK));
    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(6, 4), &list) == 0);
    assert(find_move(&list, ac_create_position(5, 5), AC_NO_SPECIAL_MOVE) == NULL);

    clear_board(&state.board);
    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_ANTEATER, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(3, 4), ac_create_piece(AC_KING, AC_BLACK));
    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(4, 4), &list) == 0);
    assert(find_move(&list, ac_create_position(3, 4), AC_ANTEATER_CAPTURE) == NULL);

    clear_board(&state.board);
    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_ROOK, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(4, 7), ac_create_piece(AC_KING, AC_BLACK));
    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(4, 4), &list) == 0);
    assert(find_move(&list, ac_create_position(4, 7), AC_NO_SPECIAL_MOVE) == NULL);

    clear_board(&state.board);
    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_QUEEN, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(4, 7), ac_create_piece(AC_KING, AC_BLACK));
    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(4, 4), &list) == 0);
    assert(find_move(&list, ac_create_position(4, 7), AC_NO_SPECIAL_MOVE) == NULL);

    clear_board(&state.board);
    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_KNIGHT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(2, 5), ac_create_piece(AC_KING, AC_BLACK));
    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(4, 4), &list) == 0);
    assert(find_move(&list, ac_create_position(2, 5), AC_NO_SPECIAL_MOVE) == NULL);

    clear_board(&state.board);
    ac_set_piece(&state.board, ac_create_position(4, 4), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(5, 5), ac_create_piece(AC_KING, AC_BLACK));
    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(4, 4), &list) == 0);
    assert(find_move(&list, ac_create_position(5, 5), AC_NO_SPECIAL_MOVE) == NULL);
}

/* Check edge-board counts and integration with selection/move validation helpers. */
static void test_edge_counts_and_validation(void) {
    AcPosition state = create_test_state(AC_WHITE);
    AcMoveList list;
    AcMove legalMove;
    AcMove invalidMove;

    ac_set_piece(&state.board, ac_create_position(0, 0), ac_create_piece(AC_ROOK, AC_WHITE));
    assert(ac_generate_legal_moves_for_position(&state, ac_create_position(0, 0), &list) == 0);
    assert(ac_get_move_count(&list) == 16);

    clear_board(&state.board);
    ac_set_piece(&state.board, ac_create_position(6, 4), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(5, 5), ac_create_piece(AC_ROOK, AC_BLACK));

    assert(ac_validate_selection(&state, ac_create_position(-1, 0)) == AC_SELECT_OUT_OF_BOUNDS);
    assert(ac_validate_selection(&state, ac_create_position(0, 0)) == AC_SELECT_EMPTY);
    assert(ac_validate_selection(&state, ac_create_position(6, 4)) == AC_SELECT_VALID);
    assert(ac_validate_selection(&state, ac_create_position(5, 5)) == AC_SELECT_OPPONENT_PIECE);

    legalMove = ac_create_move(ac_create_position(6, 4), ac_create_position(5, 4), ac_create_piece(AC_ANT, AC_WHITE));
    invalidMove =
        ac_create_move(ac_create_position(6, 4), ac_create_position(6, 10), ac_create_piece(AC_ANT, AC_WHITE));
    assert(ac_validate_move(&state, legalMove) == 1);
    assert(ac_validate_move(&state, invalidMove) == 0);
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
