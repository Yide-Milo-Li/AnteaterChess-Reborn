/* Captures independent deterministic behavior before evaluation extraction. */
#include "anteater/ai.hpp"
#include "evaluation.hpp"
#include "see.hpp"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <string.h>

using namespace ac;

static uint64_t fold(uint64_t hash, int value) {
    return (hash ^ (uint32_t)value) * UINT64_C(1099511628211);
}
static uint64_t signature(const Move *m) {
    uint64_t h = UINT64_C(1469598103934665603);
    h = fold(h, m->from.row);
    h = fold(h, m->from.col);
    h = fold(h, m->to.row);
    h = fold(h, m->to.col);
    h = fold(h, value(m->specialType));
    h = fold(h, m->pathLength);
    for (int i = 0; i < m->pathLength; ++i) {
        h = fold(h, m->path[i].row);
        h = fold(h, m->path[i].col);
    }
    h = fold(h, m->captureCount);
    for (int i = 0; i < m->captureCount; ++i) {
        h = fold(h, m->captures[i].pos.row);
        h = fold(h, m->captures[i].pos.col);
        h = fold(h, value(m->captures[i].piece.type));
        h = fold(h, value(m->captures[i].piece.color));
    }
    return h;
}
static int64_t now(void *unused) {
    (void)unused;
    return 0;
}
static void emit(const Position *p, int index) {
    auto ownedList = std::make_unique<MoveList>();
    auto *list = ownedList.get();
    if (!list || generate_legal_moves(p, list) != Status::Ok)
        exit(1);
    uint64_t see = UINT64_C(1469598103934665603);
    for (int i = 0; i < list->count; ++i) {
        see ^= signature(&list->moves[i]);
        see = fold(see, ai_see_move_score(p, &list->moves[i]));
    }
    auto owner = SearchContext::create();
    auto request = SearchRequest::create(*p, {}, {10000, 2}, Clock{now, nullptr});
    if (!std::holds_alternative<SearchContext>(owner) || !std::holds_alternative<SearchRequest>(request))
        exit(1);
    auto outcome = std::get<SearchContext>(owner).search(std::get<SearchRequest>(request));
    SearchResult r{};
    Status s = Status::Ok;
    if (auto *error = std::get_if<Error>(&outcome))
        s = error->status;
    else
        r = std::get<SearchResult>(outcome);
    printf("%d hash=%" PRIu64 " absolute=%d relative=%d moves=%d see=%" PRIu64
           " status=%d depth=%d nodes=%d move=%" PRIu64 "\n",
           index, p->hash, ai_evaluate_absolute(p), ai_evaluate_relative(p), list->count, see, value(s),
           r.completedDepth, r.nodes, s == Status::Ok ? signature(&r.move) : 0);
}
static void clear(Position *p) {
    position_init(p);
    for (int r = 0; r < Rows; ++r)
        for (int c = 0; c < Columns; ++c)
            remove_piece(&p->board, Square{r, c});
}
static void put(Position *p, int r, int c, PieceType t, Color color) {
    set_piece(&p->board, Square{r, c}, create_piece(t, color));
}
int main(void) {
    Position p;
    auto ownedMoves = std::make_unique<MoveList>();
    auto *moves = ownedMoves.get();
    uint32_t seed = 0x41c0ffee;
    position_init(&p);
    for (int i = 0; i < 24; ++i) {
        emit(&p, i);
        for (int k = 0; k < 3; ++k) {
            if (generate_legal_moves(&p, moves) != Status::Ok || !moves->count)
                break;
            seed = seed * 1664525u + 1013904223u;
            Undo undo;
            if (position_apply(&p, moves->moves[seed % moves->count], &undo) != Status::Ok)
                return 1;
        }
    }
    clear(&p);
    put(&p, 7, 5, PieceType::King, Color::White);
    put(&p, 0, 5, PieceType::King, Color::Black);
    put(&p, 3, 3, PieceType::Anteater, Color::White);
    put(&p, 2, 2, PieceType::Ant, Color::Black);
    put(&p, 2, 3, PieceType::Ant, Color::Black);
    put(&p, 1, 3, PieceType::Ant, Color::Black);
    p.hash = position_hash(&p);
    emit(&p, 24);
    clear(&p);
    put(&p, 7, 5, PieceType::King, Color::White);
    put(&p, 0, 5, PieceType::King, Color::Black);
    put(&p, 1, 2, PieceType::Ant, Color::White);
    put(&p, 0, 3, PieceType::Rook, Color::Black);
    p.hash = position_hash(&p);
    emit(&p, 25);
    clear(&p);
    put(&p, 7, 5, PieceType::King, Color::White);
    put(&p, 0, 5, PieceType::King, Color::Black);
    put(&p, 3, 4, PieceType::Ant, Color::White);
    put(&p, 3, 5, PieceType::Ant, Color::Black);
    p.enPassant = Square{3, 5};
    p.hash = position_hash(&p);
    emit(&p, 26);
    return 0;
}
