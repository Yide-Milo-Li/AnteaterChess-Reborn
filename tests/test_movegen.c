#include <assert.h>
#include <string.h>

#include "core/board.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/move.h"
#include "core/movelist.h"
#include "gameplay/movegen.h"
#include "gameplay/validation.h"

static void clear_board(Board *board) {
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            setPiece(board, createPosition(row, col), createPiece(EMPTY_PIECE, EMPTY_COLOR));
        }
    }
}

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

int main(void) {
    test_generate_moves_only_for_current_turn();
    test_ant_moves_and_capture();
    test_anteater_moves_and_chain_capture();
    test_sliding_piece_blocking();
    test_bishop_and_queen_generation();
    test_knight_and_king_moves();
    test_movegen_does_not_generate_king_captures();
    test_edge_counts_and_validation();
    return 0;
}
