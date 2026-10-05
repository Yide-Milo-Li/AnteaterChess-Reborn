#include "anteater/rules.h"
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>

static void clear_board(AcPosition *state) {
    int row;
    int col;

    assert(state != NULL);
    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            ac_set_piece(&state->board, ac_create_position(row, col), ac_create_piece(AC_EMPTY_PIECE, AC_EMPTY_COLOR));
        }
    }
}

static AcPosition fresh_empty_state(AcColor turn) {
    AcGameConfig config;
    AcPosition state;

    ac_init_default_game_config(&config);
    ac_position_init(&state);
    clear_board(&state);
    state.currentTurn = turn;
    state.castlingRights = 15;
    state.enPassant = ac_create_position(-1, -1);
    state.moveCount = 0;

    ac_set_piece(&state.board, ac_create_position(7, 5), ac_create_piece(AC_KING, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(0, 5), ac_create_piece(AC_KING, AC_BLACK));
    return state;
}

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

static void test_resolve_simple_move(void) {
    AcPosition state = fresh_empty_state(AC_WHITE);
    AcMoveRequest request;
    AcMove move;

    ac_set_piece(&state.board, ac_create_position(6, 0), ac_create_piece(AC_ANT, AC_WHITE));
    assert(ac_create_move_request(&request, ac_create_position(6, 0), ac_create_position(5, 0),
                                  AC_PROMOTION_CHOICE_NONE) == 0);

    assert(ac_resolve_move_request(&state, request, &move) == 0);
    assert(ac_position_equal(move.from, ac_create_position(6, 0)));
    assert(ac_position_equal(move.to, ac_create_position(5, 0)));
    assert(move.specialType == AC_NO_SPECIAL_MOVE);
}

static void test_resolve_castling(void) {
    AcPosition state = fresh_empty_state(AC_WHITE);
    AcMoveRequest request;
    AcMove move;

    ac_set_piece(&state.board, ac_create_position(7, 9), ac_create_piece(AC_ROOK, AC_WHITE));
    assert(ac_create_move_request(&request, ac_create_position(7, 5), ac_create_position(7, 7),
                                  AC_PROMOTION_CHOICE_NONE) == 0);

    assert(ac_resolve_move_request(&state, request, &move) == 0);
    assert(move.specialType == AC_CASTLING_KINGSIDE);
}

static void test_resolve_en_passant(void) {
    AcPosition state = fresh_empty_state(AC_WHITE);
    AcMoveRequest request;
    AcMove move;

    ac_set_piece(&state.board, ac_create_position(3, 4), ac_create_piece(AC_ANT, AC_WHITE));
    ac_set_piece(&state.board, ac_create_position(3, 5), ac_create_piece(AC_ANT, AC_BLACK));
    push_history_move(
        &state, ac_create_move(ac_create_position(1, 5), ac_create_position(3, 5), ac_create_piece(AC_ANT, AC_BLACK)));

    assert(ac_create_move_request(&request, ac_create_position(3, 4), ac_create_position(2, 5),
                                  AC_PROMOTION_CHOICE_NONE) == 0);

    assert(ac_resolve_move_request(&state, request, &move) == 0);
    assert(move.specialType == AC_EN_PASSANT);
    assert(move.captureCount == 1);
    assert(ac_position_equal(move.captures[0].pos, ac_create_position(3, 5)));
}

static void test_resolve_promotion_defaults_to_queen(void) {
    AcPosition state = fresh_empty_state(AC_WHITE);
    AcMoveRequest request;
    AcMove move;

    ac_set_piece(&state.board, ac_create_position(1, 2), ac_create_piece(AC_ANT, AC_WHITE));
    assert(ac_create_move_request(&request, ac_create_position(1, 2), ac_create_position(0, 2),
                                  AC_PROMOTION_CHOICE_NONE) == 0);

    assert(ac_resolve_move_request(&state, request, &move) == 0);
    assert(move.specialType == AC_PROMOTION_QUEEN);
}

static void test_resolve_explicit_promotion_choices(void) {
    AcPosition state = fresh_empty_state(AC_WHITE);
    AcMoveRequest request;
    AcMove move;

    ac_set_piece(&state.board, ac_create_position(1, 2), ac_create_piece(AC_ANT, AC_WHITE));

    assert(ac_create_move_request(&request, ac_create_position(1, 2), ac_create_position(0, 2),
                                  AC_PROMOTION_CHOICE_ROOK) == 0);
    assert(ac_resolve_move_request(&state, request, &move) == 0);
    assert(move.specialType == AC_PROMOTION_ROOK);

    request.promotion = AC_PROMOTION_CHOICE_BISHOP;
    assert(ac_resolve_move_request(&state, request, &move) == 0);
    assert(move.specialType == AC_PROMOTION_BISHOP);

    request.promotion = AC_PROMOTION_CHOICE_KNIGHT;
    assert(ac_resolve_move_request(&state, request, &move) == 0);
    assert(move.specialType == AC_PROMOTION_KNIGHT);
}

static void test_resolve_rejects_invalid_request(void) {
    AcPosition state = fresh_empty_state(AC_WHITE);
    AcMoveRequest request;
    AcMove move;

    ac_set_piece(&state.board, ac_create_position(1, 2), ac_create_piece(AC_ANT, AC_WHITE));
    assert(ac_create_move_request(&request, ac_create_position(1, 2), ac_create_position(0, 2),
                                  AC_PROMOTION_CHOICE_QUEEN) == 0);
    request.promotion = (AcPromotionChoice)99;

    assert(ac_resolve_move_request(&state, request, &move) != 0);
}

int main(void) {
    test_resolve_simple_move();
    test_resolve_castling();
    test_resolve_en_passant();
    test_resolve_promotion_defaults_to_queen();
    test_resolve_explicit_promotion_choices();
    test_resolve_rejects_invalid_request();
    return 0;
}
