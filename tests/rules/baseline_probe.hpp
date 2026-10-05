// Maintained C++ driver extracted from the frozen historical probe.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "anteater/rules.hpp"

using namespace ac;
typedef Position State;
typedef MoveList List;
typedef Move M;
#define gen generate_legal_moves
#define square create_position
#define piece(type, color) create_piece(static_cast<PieceType>(type), static_cast<Color>(color))
#define place set_piece
static int apply(State *s, M m) {
    Undo u;
    return value(position_apply(s, m, &u));
}
static void init(State *s) {
    position_init(s);
}
static int result(State *s) {
    GameResult r;
    assert(!position_result(s, &r));
    return value(r);
}
static uint64_t fingerprint(const List *l) {
    uint64_t sum = 0;
    for (int i = 0; i < l->count; ++i) {
        M m = l->moves[i];
        uint64_t h = 1469598103934665603ULL;
        int values[100], n = 0;
        values[n++] = m.from.row;
        values[n++] = m.from.col;
        values[n++] = m.to.row;
        values[n++] = m.to.col;
        values[n++] = value(m.specialType);
        values[n++] = m.captureCount;
        values[n++] = m.pathLength;
        for (int k = 0; k < m.captureCount; ++k) {
            values[n++] = m.captures[k].pos.row;
            values[n++] = m.captures[k].pos.col;
            values[n++] = value(m.captures[k].piece.type);
            values[n++] = value(m.captures[k].piece.color);
        }
        for (int k = 0; k < m.pathLength; ++k) {
            values[n++] = m.path[k].row;
            values[n++] = m.path[k].col;
        }
        for (int k = 0; k < n; ++k) {
            h ^= (unsigned)values[k];
            h *= 1099511628211ULL;
        }
        sum += h;
    }
    return sum;
}
static unsigned long long perft(State *s, int depth) {
    if (!depth)
        return 1;
    List *l = static_cast<List *>(malloc(sizeof(*l)));
    State *next = static_cast<State *>(malloc(sizeof(*next)));
    assert(l && next);
    assert(!gen(s, l));
    unsigned long long n = 0;
    for (int i = 0; i < l->count; ++i) {
        *next = *s;
        assert(!apply(next, l->moves[i]));
        n += perft(next, depth - 1);
    }
    free(next);
    free(l);
    return n;
}
static void probe(FILE *out) {
    State *s = static_cast<State *>(malloc(sizeof(*s)));
    List *l = static_cast<List *>(malloc(sizeof(*l)));
    assert(s && l);
    init(s);
    for (int depth = 1; depth <= 3; ++depth)
        fprintf(out, "perft %d %llu\n", depth, perft(s, depth));
    unsigned random = 47;
    for (int i = 0; i < 50; ++i) {
        assert(!gen(s, l));
        fprintf(out, "line %d %d %llu %d\n", i, l->count, (unsigned long long)fingerprint(l), result(s));
        if (!l->count)
            break;
        random = random * 1664525u + 1013904223u;
        assert(!apply(s, l->moves[random % (unsigned)l->count]));
    }
    for (int scenario = 0; scenario < 5; ++scenario) {
        init(s);
        for (int r = 0; r < 8; ++r)
            for (int c = 0; c < 10; ++c)
                place(&s->board, square(r, c), piece(7, 2));
        place(&s->board, square(7, 5), piece(5, 0));
        place(&s->board, square(0, 5), piece(5, 1));
        if (scenario == 0) {
            place(&s->board, square(7, 0), piece(1, 0));
            place(&s->board, square(7, 9), piece(1, 0));
        }
        if (scenario == 1) {
            place(&s->board, square(4, 4), piece(6, 0));
            place(&s->board, square(3, 4), piece(0, 1));
            place(&s->board, square(3, 5), piece(0, 1));
            place(&s->board, square(4, 5), piece(0, 1));
        }
        if (scenario == 2) {
            place(&s->board, square(1, 2), piece(0, 0));
            place(&s->board, square(0, 3), piece(1, 1));
        }
        if (scenario == 3) {
            place(&s->board, square(4, 0), piece(3, 0));
            place(&s->board, square(4, 8), piece(3, 1));
        }
        if (scenario == 4) {
            place(&s->board, square(6, 5), piece(1, 1));
        }
#ifndef AC_LEGACY_PROBE
        s->hash = position_hash(s);
#endif
        assert(!gen(s, l));
        fprintf(out, "fixture %d %d %llu %d\n", scenario, l->count, (unsigned long long)fingerprint(l), result(s));
    }
    free(s);
    free(l);
}
