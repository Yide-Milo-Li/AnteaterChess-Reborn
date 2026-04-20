#include <assert.h>
#include <string.h>

#include "core/board.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/move.h"
#include "core/movelist.h"
#include "gameplay/movegen.h"
#include "gameplay/validation.h"

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

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            setPiece(board, createPosition(row, col), createPiece(EMPTY_PIECE, EMPTY_COLOR));
        }
    }
}

/* Build a minimal GameState with an empty board and an explicit side to move. */
static GameState create_test_state(Color turn) {
    GameState state;

    memset(&state, 0, sizeof(state));
    initDefaultGameConfig(&state.config);
    initBoard(&state.board);
    clear_board(&state.board);
    initMoveList(&state.moveHistory);
    state.currentTurn = turn;
    return state;
}

/* Find a generated move by destination square and special-move tag. */
static Move *find_move(MoveList *list, Position to, SpecialMove specialType) {
    int index;

    for (index = 0; index < getMoveCount(list); ++index) {
        Move *move = getMove(list, index);

        if (move != NULL && positionEqual(move->to, to) && move->specialType == specialType) {
            return move;
        }
    }

    return NULL;
}

/* Record one historical move without mutating the current board state. */
static void push_history_move(GameState *state, Move move) {
    assert(state != NULL);
    assert(addMove(&state->moveHistory, move) == 0);
    ++state->moveCount;
}

/* Check that generation only considers pieces belonging to the side to move. */
static void test_generate_moves_only_for_current_turn(void) {
    GameState state = create_test_state(WHITE);
    MoveList list;

    setPiece(&state.board, createPosition(6, 4), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(1, 4), createPiece(ANT, BLACK));

    assert(generateMoves(&state, &list) == 0);
    assert(getMoveCount(&list) == 2);
    assert(find_move(&list, createPosition(5, 4), NO_SPECIAL_MOVE) != NULL);
    assert(find_move(&list, createPosition(4, 4), NO_SPECIAL_MOVE) != NULL);
}

/* Check ant forward movement, opening double-step, and diagonal capture rules. */
static void test_ant_moves_and_capture(void) {
    GameState state = create_test_state(WHITE);
    MoveList list;
    Move *captureMove;

    setPiece(&state.board, createPosition(6, 4), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(5, 5), createPiece(KNIGHT, BLACK));
    setPiece(&state.board, createPosition(5, 3), createPiece(BISHOP, WHITE));

    assert(generateLegalMovesForPosition(&state, createPosition(6, 4), &list) == 0);
    assert(find_move(&list, createPosition(5, 4), NO_SPECIAL_MOVE) != NULL);
    assert(find_move(&list, createPosition(4, 4), NO_SPECIAL_MOVE) != NULL);

    captureMove = find_move(&list, createPosition(5, 5), NO_SPECIAL_MOVE);
    assert(captureMove != NULL);
    assert(captureMove->captureCount == 1);
    assert(captureMove->captures[0].piece.type == KNIGHT);
    assert(find_move(&list, createPosition(5, 3), NO_SPECIAL_MOVE) == NULL);
}

/* Check anteater step moves and orthogonal chain-capture generation. */
static void test_anteater_moves_and_chain_capture(void) {
    GameState state = create_test_state(WHITE);
    MoveList list;
    Move *stepMove;
    Move *singleCapture;
    Move *chainCapture;

    setPiece(&state.board, createPosition(4, 4), createPiece(ANTEATER, WHITE));
    setPiece(&state.board, createPosition(3, 4), createPiece(ANT, BLACK));
    setPiece(&state.board, createPosition(4, 5), createPiece(ANT, BLACK));
    setPiece(&state.board, createPosition(4, 6), createPiece(ANT, BLACK));

    assert(generateLegalMovesForPosition(&state, createPosition(4, 4), &list) == 0);

    stepMove = find_move(&list, createPosition(3, 3), NO_SPECIAL_MOVE);
    assert(stepMove != NULL);

    singleCapture = find_move(&list, createPosition(3, 4), ANTEATER_CAPTURE);
    assert(singleCapture != NULL);
    assert(singleCapture->captureCount == 1);
    assert(singleCapture->pathLength == 0);

    chainCapture = find_move(&list, createPosition(4, 6), ANTEATER_CAPTURE);
    assert(chainCapture != NULL);
    assert(chainCapture->captureCount == 2);
    assert(chainCapture->pathLength == 2);
    assert(positionEqual(chainCapture->path[0], createPosition(4, 5)) == 1);
    assert(positionEqual(chainCapture->path[1], createPosition(4, 6)) == 1);
}

/* Check that sliding pieces stop at the first blocker in each direction. */
static void test_sliding_piece_blocking(void) {
    GameState state = create_test_state(WHITE);
    MoveList list;

    setPiece(&state.board, createPosition(4, 4), createPiece(ROOK, WHITE));
    setPiece(&state.board, createPosition(2, 4), createPiece(ANT, BLACK));
    setPiece(&state.board, createPosition(4, 6), createPiece(ANT, WHITE));

    assert(generateLegalMovesForPosition(&state, createPosition(4, 4), &list) == 0);
    assert(find_move(&list, createPosition(3, 4), NO_SPECIAL_MOVE) != NULL);
    assert(find_move(&list, createPosition(2, 4), NO_SPECIAL_MOVE) != NULL);
    assert(find_move(&list, createPosition(1, 4), NO_SPECIAL_MOVE) == NULL);
    assert(find_move(&list, createPosition(4, 6), NO_SPECIAL_MOVE) == NULL);
}

/* Check combined queen movement and bishop-style diagonal generation. */
static void test_bishop_and_queen_generation(void) {
    GameState state = create_test_state(WHITE);
    MoveList list;

    setPiece(&state.board, createPosition(4, 4), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(6, 6), createPiece(ROOK, BLACK));
    setPiece(&state.board, createPosition(4, 6), createPiece(BISHOP, WHITE));

    assert(generateLegalMovesForPosition(&state, createPosition(4, 4), &list) == 0);
    assert(find_move(&list, createPosition(4, 5), NO_SPECIAL_MOVE) != NULL);
    assert(find_move(&list, createPosition(4, 6), NO_SPECIAL_MOVE) == NULL);
    assert(find_move(&list, createPosition(5, 5), NO_SPECIAL_MOVE) != NULL);
    assert(find_move(&list, createPosition(6, 6), NO_SPECIAL_MOVE) != NULL);
}

/* Check knight jump captures and king single-step landing rules. */
static void test_knight_and_king_moves(void) {
    GameState state = create_test_state(WHITE);
    MoveList knightList;
    MoveList kingList;
    Move *captureMove;

    setPiece(&state.board, createPosition(4, 4), createPiece(KNIGHT, WHITE));
    setPiece(&state.board, createPosition(4, 5), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(3, 4), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(2, 5), createPiece(BISHOP, BLACK));

    assert(generateLegalMovesForPosition(&state, createPosition(4, 4), &knightList) == 0);
    captureMove = find_move(&knightList, createPosition(2, 5), NO_SPECIAL_MOVE);
    assert(captureMove != NULL);
    assert(captureMove->captureCount == 1);

    clear_board(&state.board);
    setPiece(&state.board, createPosition(4, 4), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(4, 5), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(5, 5), createPiece(ANT, BLACK));

    assert(generateLegalMovesForPosition(&state, createPosition(4, 4), &kingList) == 0);
    assert(find_move(&kingList, createPosition(4, 5), NO_SPECIAL_MOVE) == NULL);
    assert(find_move(&kingList, createPosition(5, 5), NO_SPECIAL_MOVE) != NULL);
}

/* Check that promotion candidates are generated as four variants for forward
 * and capture cases, for both colors. */
static void test_promotion_generation(void) {
    GameState whiteState = create_test_state(WHITE);
    GameState blackState = create_test_state(BLACK);
    MoveList list;
    Move *move;

    setPiece(&whiteState.board, createPosition(1, 4), createPiece(ANT, WHITE));
    setPiece(&whiteState.board, createPosition(0, 5), createPiece(ROOK, BLACK));

    assert(generateLegalMovesForPosition(&whiteState, createPosition(1, 4), &list) == 0);
    assert(find_move(&list, createPosition(0, 4), NO_SPECIAL_MOVE) == NULL);
    assert(find_move(&list, createPosition(0, 4), PROMOTION_QUEEN) != NULL);
    assert(find_move(&list, createPosition(0, 4), PROMOTION_ROOK) != NULL);
    assert(find_move(&list, createPosition(0, 4), PROMOTION_BISHOP) != NULL);
    assert(find_move(&list, createPosition(0, 4), PROMOTION_KNIGHT) != NULL);

    move = find_move(&list, createPosition(0, 5), PROMOTION_QUEEN);
    assert(move != NULL);
    assert(move->captureCount == 1);
    assert(move->captures[0].piece.type == ROOK);

    setPiece(&blackState.board, createPosition(6, 4), createPiece(ANT, BLACK));
    assert(generateLegalMovesForPosition(&blackState, createPosition(6, 4), &list) == 0);
    assert(find_move(&list, createPosition(7, 4), PROMOTION_QUEEN) != NULL);
    assert(find_move(&list, createPosition(7, 4), PROMOTION_ROOK) != NULL);
    assert(find_move(&list, createPosition(7, 4), PROMOTION_BISHOP) != NULL);
    assert(find_move(&list, createPosition(7, 4), PROMOTION_KNIGHT) != NULL);
}

/* Check castling generation, plus the main blocking and attack-based rejection
 * cases reconstructed from board state and history. */
static void test_castling_generation(void) {
    GameState state = create_test_state(WHITE);
    MoveList list;

    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(7, 0), createPiece(ROOK, WHITE));
    setPiece(&state.board, createPosition(7, 9), createPiece(ROOK, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));

    assert(generateLegalMovesForPosition(&state, createPosition(7, 5), &list) == 0);
    assert(find_move(&list, createPosition(7, 7), CASTLING_KINGSIDE) != NULL);
    assert(find_move(&list, createPosition(7, 3), CASTLING_QUEENSIDE) != NULL);

    setPiece(&state.board, createPosition(7, 6), createPiece(BISHOP, WHITE));
    assert(generateLegalMovesForPosition(&state, createPosition(7, 5), &list) == 0);
    assert(find_move(&list, createPosition(7, 7), CASTLING_KINGSIDE) == NULL);

    clear_board(&state.board);
    initMoveList(&state.moveHistory);
    state.moveCount = 0;
    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(7, 0), createPiece(ROOK, WHITE));
    setPiece(&state.board, createPosition(7, 9), createPiece(ROOK, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(5, 6), createPiece(ROOK, BLACK));
    assert(generateLegalMovesForPosition(&state, createPosition(7, 5), &list) == 0);
    assert(find_move(&list, createPosition(7, 7), CASTLING_KINGSIDE) == NULL);

    clear_board(&state.board);
    initMoveList(&state.moveHistory);
    state.moveCount = 0;
    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(7, 0), createPiece(ROOK, WHITE));
    setPiece(&state.board, createPosition(7, 9), createPiece(ROOK, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    push_history_move(&state, createMove(createPosition(7, 9), createPosition(7, 8), createPiece(ROOK, WHITE)));
    assert(generateLegalMovesForPosition(&state, createPosition(7, 5), &list) == 0);
    assert(find_move(&list, createPosition(7, 7), CASTLING_KINGSIDE) == NULL);
}

/* Check en passant generation from the latest double-step ant move only. */
static void test_en_passant_generation(void) {
    GameState state = create_test_state(WHITE);
    MoveList list;
    Move lastMove;
    Move *epMove;

    setPiece(&state.board, createPosition(3, 4), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(3, 5), createPiece(ANT, BLACK));

    lastMove = createMove(createPosition(1, 5), createPosition(3, 5), createPiece(ANT, BLACK));
    push_history_move(&state, lastMove);

    assert(generateLegalMovesForPosition(&state, createPosition(3, 4), &list) == 0);
    epMove = find_move(&list, createPosition(2, 5), EN_PASSANT);
    assert(epMove != NULL);
    assert(epMove->captureCount == 1);
    assert(positionEqual(epMove->captures[0].pos, createPosition(3, 5)) == 1);

    initMoveList(&state.moveHistory);
    state.moveCount = 0;
    lastMove = createMove(createPosition(2, 5), createPosition(3, 5), createPiece(ANT, BLACK));
    push_history_move(&state, lastMove);
    assert(generateLegalMovesForPosition(&state, createPosition(3, 4), &list) == 0);
    assert(find_move(&list, createPosition(2, 5), EN_PASSANT) == NULL);
}

/* Check that no piece generator emits a direct capture onto an enemy king square. */
static void test_movegen_does_not_generate_king_captures(void) {
    GameState state = create_test_state(WHITE);
    MoveList list;

    setPiece(&state.board, createPosition(6, 4), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(5, 5), createPiece(KING, BLACK));
    assert(generateLegalMovesForPosition(&state, createPosition(6, 4), &list) == 0);
    assert(find_move(&list, createPosition(5, 5), NO_SPECIAL_MOVE) == NULL);

    clear_board(&state.board);
    setPiece(&state.board, createPosition(4, 4), createPiece(ANTEATER, WHITE));
    setPiece(&state.board, createPosition(3, 4), createPiece(KING, BLACK));
    assert(generateLegalMovesForPosition(&state, createPosition(4, 4), &list) == 0);
    assert(find_move(&list, createPosition(3, 4), ANTEATER_CAPTURE) == NULL);

    clear_board(&state.board);
    setPiece(&state.board, createPosition(4, 4), createPiece(ROOK, WHITE));
    setPiece(&state.board, createPosition(4, 7), createPiece(KING, BLACK));
    assert(generateLegalMovesForPosition(&state, createPosition(4, 4), &list) == 0);
    assert(find_move(&list, createPosition(4, 7), NO_SPECIAL_MOVE) == NULL);

    clear_board(&state.board);
    setPiece(&state.board, createPosition(4, 4), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(4, 7), createPiece(KING, BLACK));
    assert(generateLegalMovesForPosition(&state, createPosition(4, 4), &list) == 0);
    assert(find_move(&list, createPosition(4, 7), NO_SPECIAL_MOVE) == NULL);

    clear_board(&state.board);
    setPiece(&state.board, createPosition(4, 4), createPiece(KNIGHT, WHITE));
    setPiece(&state.board, createPosition(2, 5), createPiece(KING, BLACK));
    assert(generateLegalMovesForPosition(&state, createPosition(4, 4), &list) == 0);
    assert(find_move(&list, createPosition(2, 5), NO_SPECIAL_MOVE) == NULL);

    clear_board(&state.board);
    setPiece(&state.board, createPosition(4, 4), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(5, 5), createPiece(KING, BLACK));
    assert(generateLegalMovesForPosition(&state, createPosition(4, 4), &list) == 0);
    assert(find_move(&list, createPosition(5, 5), NO_SPECIAL_MOVE) == NULL);
}

/* Check edge-board counts and integration with selection/move validation helpers. */
static void test_edge_counts_and_validation(void) {
    GameState state = create_test_state(WHITE);
    MoveList list;
    Move legalMove;
    Move invalidMove;

    setPiece(&state.board, createPosition(0, 0), createPiece(ROOK, WHITE));
    assert(generateLegalMovesForPosition(&state, createPosition(0, 0), &list) == 0);
    assert(getMoveCount(&list) == 16);

    clear_board(&state.board);
    setPiece(&state.board, createPosition(6, 4), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(5, 5), createPiece(ROOK, BLACK));

    assert(validateSelection(&state, createPosition(-1, 0)) == SELECT_OUT_OF_BOUNDS);
    assert(validateSelection(&state, createPosition(0, 0)) == SELECT_EMPTY);
    assert(validateSelection(&state, createPosition(6, 4)) == SELECT_VALID);
    assert(validateSelection(&state, createPosition(5, 5)) == SELECT_OPPONENT_PIECE);

    legalMove = createMove(createPosition(6, 4), createPosition(5, 4), createPiece(ANT, WHITE));
    invalidMove = createMove(createPosition(6, 4), createPosition(6, 10), createPiece(ANT, WHITE));
    assert(validateMove(&state, legalMove) == 1);
    assert(validateMove(&state, invalidMove) == 0);
}

/* Run the move-generation regression suite for the supported piece rules. */
int main(void) {
    test_generate_moves_only_for_current_turn();
    test_ant_moves_and_capture();
    test_anteater_moves_and_chain_capture();
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
