#include "anteater/rules.hpp"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

using namespace ac;
static Move resolve(Position *p, const char *from, const char *to) {
    MoveRequest r;
    Move m;
    assert(!parse_move_request_fields(from, to, PromotionChoice::Queen, &r));
    assert(!resolve_move_request(p, r, &m));
    return m;
}
static void empty(Position *p) {
    position_init(p);
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 10; ++c)
            remove_piece(&p->board, create_position(r, c));
}
static void place(Position *p, const char *sq, PieceType type, Color c) {
    set_piece(&p->board, parse_position(sq), create_piece(type, c));
    p->hash = position_hash(p);
}
int main(void) {
    Position p, initial;
    position_init(&p);
    initial = p;
    Undo undo[200];
    unsigned rng = 12345;
    MoveList *moves = static_cast<MoveList *>(malloc(sizeof(*moves)));
    assert(moves);
    int played = 0;
    for (; played < 150; ++played) {
        assert(!generate_legal_moves(&p, moves));
        if (!moves->count)
            break;
        rng = rng * 1664525u + 1013904223u;
        assert(!position_apply(&p, moves->moves[rng % (unsigned)moves->count], &undo[played]));
        assert(p.hash == position_hash(&p));
    }
    while (played > 0) {
        --played;
        assert(!position_unmake(&p, &undo[played]));
        assert(p.hash == position_hash(&p));
    }
    assert((p == initial));
    Move bad = create_move(parse_position("A1"), parse_position("J8"), create_piece(PieceType::Rook, Color::White));
    assert(position_apply(&p, bad, &undo[0]) != Status::Ok);
    assert((p == initial));
    empty(&p);
    place(&p, "F1", PieceType::King, Color::White);
    place(&p, "F8", PieceType::King, Color::Black);
    place(&p, "J1", PieceType::Rook, Color::White);
    initial = p;
    Move castle = resolve(&p, "F1", "H1");
    assert(castle.specialType == SpecialMove::CastlingKingside);
    assert(!position_apply(&p, castle, &undo[0]));
    assert(p.castlingRights == 12);
    assert(!position_unmake(&p, &undo[0]));
    assert((p == initial));
    empty(&p);
    place(&p, "A1", PieceType::King, Color::White);
    place(&p, "J8", PieceType::King, Color::Black);
    place(&p, "C7", PieceType::Ant, Color::White);
    Move promotion = resolve(&p, "C7", "C8");
    assert(promotion.specialType == SpecialMove::PromotionQueen);
    initial = p;
    assert(!position_apply(&p, promotion, &undo[0]));
    assert(!position_unmake(&p, &undo[0]));
    assert((p == initial));
    empty(&p);
    place(&p, "A1", PieceType::King, Color::White);
    place(&p, "J8", PieceType::King, Color::Black);
    place(&p, "E5", PieceType::Ant, Color::White);
    place(&p, "F7", PieceType::Ant, Color::Black);
    p.currentTurn = Color::Black;
    p.hash = position_hash(&p);
    Move push = resolve(&p, "F7", "F5");
    assert(!position_apply(&p, push, &undo[0]));
    Move ep = resolve(&p, "E5", "F6");
    assert(ep.specialType == SpecialMove::EnPassant);
    initial = p;
    assert(!position_apply(&p, ep, &undo[1]));
    assert(!position_unmake(&p, &undo[1]));
    assert((p == initial));
    empty(&p);
    place(&p, "A1", PieceType::King, Color::White);
    place(&p, "J8", PieceType::King, Color::Black);
    place(&p, "D4", PieceType::Anteater, Color::White);
    place(&p, "E4", PieceType::Ant, Color::Black);
    place(&p, "E5", PieceType::Ant, Color::Black);
    MoveRequest req;
    Move chain;
    assert(!parse_move_request_fields("D4", "E5", PromotionChoice::None, &req));
    assert(resolve_move_request(&p, req, &chain) != Status::Ok);
    assert(!generate_legal_moves_for_position(&p, parse_position("D4"), moves));
    int found = 0;
    for (int i = 0; i < moves->count; ++i)
        if (moves->moves[i].captureCount == 2) {
            initial = p;
            assert(!position_apply(&p, moves->moves[i], &undo[0]));
            assert(!position_unmake(&p, &undo[0]));
            assert((p == initial));
            found = 1;
        }
    assert(found);
    init_move_list(moves);
    for (int i = 0; i < MaxMoves; ++i)
        assert(!add_move(moves, bad));
    assert(add_move(moves, bad) == Status::Capacity && moves->status == Status::Capacity);
    free(moves);
    return 0;
}
