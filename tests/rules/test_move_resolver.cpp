#include "anteater/rules.hpp"
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>

using namespace ac;

static void clear_board(Position *state) {
    int row;
    int col;

    assert(state != NULL);
    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            set_piece(&state->board, create_position(row, col), create_piece(PieceType::Empty, Color::Empty));
        }
    }
}

static Position fresh_empty_state(Color turn) {
    GameConfig config;
    Position state;

    init_default_game_config(&config);
    position_init(&state);
    clear_board(&state);
    state.currentTurn = turn;
    state.castlingRights = 15;
    state.enPassant = create_position(-1, -1);
    state.moveCount = 0;

    set_piece(&state.board, create_position(7, 5), create_piece(PieceType::King, Color::White));
    set_piece(&state.board, create_position(0, 5), create_piece(PieceType::King, Color::Black));
    return state;
}

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

static void test_resolve_simple_move(void) {
    Position state = fresh_empty_state(Color::White);
    MoveRequest request;
    Move move;

    set_piece(&state.board, create_position(6, 0), create_piece(PieceType::Ant, Color::White));
    assert(create_move_request(&request, create_position(6, 0), create_position(5, 0), PromotionChoice::None) ==
           Status::Ok);

    assert(resolve_move_request(&state, request, &move) == Status::Ok);
    assert(position_equal(move.from, create_position(6, 0)));
    assert(position_equal(move.to, create_position(5, 0)));
    assert(move.specialType == SpecialMove::None);
}

static void test_resolve_castling(void) {
    Position state = fresh_empty_state(Color::White);
    MoveRequest request;
    Move move;

    set_piece(&state.board, create_position(7, 9), create_piece(PieceType::Rook, Color::White));
    assert(create_move_request(&request, create_position(7, 5), create_position(7, 7), PromotionChoice::None) ==
           Status::Ok);

    assert(resolve_move_request(&state, request, &move) == Status::Ok);
    assert(move.specialType == SpecialMove::CastlingKingside);
}

static void test_resolve_en_passant(void) {
    Position state = fresh_empty_state(Color::White);
    MoveRequest request;
    Move move;

    set_piece(&state.board, create_position(3, 4), create_piece(PieceType::Ant, Color::White));
    set_piece(&state.board, create_position(3, 5), create_piece(PieceType::Ant, Color::Black));
    push_history_move(
        &state, create_move(create_position(1, 5), create_position(3, 5), create_piece(PieceType::Ant, Color::Black)));

    assert(create_move_request(&request, create_position(3, 4), create_position(2, 5), PromotionChoice::None) ==
           Status::Ok);

    assert(resolve_move_request(&state, request, &move) == Status::Ok);
    assert(move.specialType == SpecialMove::EnPassant);
    assert(move.captureCount == 1);
    assert(position_equal(move.captures[0].pos, create_position(3, 5)));
}

static void test_resolve_promotion_defaults_to_queen(void) {
    Position state = fresh_empty_state(Color::White);
    MoveRequest request;
    Move move;

    set_piece(&state.board, create_position(1, 2), create_piece(PieceType::Ant, Color::White));
    assert(create_move_request(&request, create_position(1, 2), create_position(0, 2), PromotionChoice::None) ==
           Status::Ok);

    assert(resolve_move_request(&state, request, &move) == Status::Ok);
    assert(move.specialType == SpecialMove::PromotionQueen);
}

static void test_resolve_explicit_promotion_choices(void) {
    Position state = fresh_empty_state(Color::White);
    MoveRequest request;
    Move move;

    set_piece(&state.board, create_position(1, 2), create_piece(PieceType::Ant, Color::White));

    assert(create_move_request(&request, create_position(1, 2), create_position(0, 2), PromotionChoice::Rook) ==
           Status::Ok);
    assert(resolve_move_request(&state, request, &move) == Status::Ok);
    assert(move.specialType == SpecialMove::PromotionRook);

    request.promotion = PromotionChoice::Bishop;
    assert(resolve_move_request(&state, request, &move) == Status::Ok);
    assert(move.specialType == SpecialMove::PromotionBishop);

    request.promotion = PromotionChoice::Knight;
    assert(resolve_move_request(&state, request, &move) == Status::Ok);
    assert(move.specialType == SpecialMove::PromotionKnight);
}

static void test_resolve_rejects_invalid_request(void) {
    Position state = fresh_empty_state(Color::White);
    MoveRequest request;
    Move move;

    set_piece(&state.board, create_position(1, 2), create_piece(PieceType::Ant, Color::White));
    assert(create_move_request(&request, create_position(1, 2), create_position(0, 2), PromotionChoice::Queen) ==
           Status::Ok);
    request.promotion = (PromotionChoice)99;

    assert(resolve_move_request(&state, request, &move) != Status::Ok);
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
