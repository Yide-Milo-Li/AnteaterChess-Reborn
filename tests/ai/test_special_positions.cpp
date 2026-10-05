#include "anteater/ai.hpp"
#include <assert.h>
#include <string.h>

using namespace ac;
static int64_t now(void *data) {
    return (*(int64_t *)data)++;
}
static void empty(Position *p) {
    position_init(p);
    for (int r = 0; r < Rows; ++r)
        for (int c = 0; c < Columns; ++c)
            remove_piece(&p->board, create_position(r, c));
}
static void put(Position *p, int r, int c, PieceType type, Color color) {
    set_piece(&p->board, create_position(r, c), create_piece(type, color));
}
int main(void) {
    SearchContext *context = search_create();
    assert(context);
    int64_t ms = 0;
    SearchOptions o = {
        {now, &ms},
        500, 3, NULL, NULL, NULL, 0
    };
    SearchResult result;
    Position p, before;
    Undo undo;
    empty(&p);
    put(&p, 7, 5, PieceType::King, Color::White);
    put(&p, 7, 0, PieceType::Rook, Color::White);
    put(&p, 6, 4, PieceType::Ant, Color::White);
    put(&p, 5, 5, PieceType::Rook, Color::Black);
    put(&p, 0, 0, PieceType::King, Color::Black);
    p.hash = position_hash(&p);
    before = p;
    assert(is_in_check(&p, Color::White));
    assert(!search(context, &p, &o, &result));
    assert((p == before));
    assert(!position_apply(&p, result.move, &undo));
    assert(!is_in_check(&p, Color::White));
    empty(&p);
    p.currentTurn = Color::Black;
    put(&p, 0, 0, PieceType::King, Color::Black);
    put(&p, 1, 1, PieceType::Queen, Color::White);
    put(&p, 2, 2, PieceType::King, Color::White);
    p.hash = position_hash(&p);
    assert(search(context, &p, &o, &result) == Status::Unavailable);
    empty(&p);
    put(&p, 1, 2, PieceType::Ant, Color::White);
    put(&p, 0, 2, PieceType::Bishop, Color::Black);
    put(&p, 0, 3, PieceType::Rook, Color::Black);
    p.hash = position_hash(&p);
    assert(!search(context, &p, &o, &result));
    assert(is_promotion_special_move(result.move.specialType));
    assert(!position_apply(&p, result.move, &undo));
    empty(&p);
    put(&p, 3, 4, PieceType::Ant, Color::White);
    put(&p, 3, 5, PieceType::Ant, Color::Black);
    put(&p, 2, 4, PieceType::Rook, Color::Black);
    p.enPassant = create_position(3, 5);
    p.hash = position_hash(&p);
    assert(!search(context, &p, &o, &result));
    assert(result.move.specialType == SpecialMove::EnPassant);
    assert(!position_apply(&p, result.move, &undo));
    search_destroy(context);
    return 0;
}
