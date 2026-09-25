#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifdef AC_LEGACY_PROBE
#include "core/gamestate.h"
#include "gameplay/movegen.h"
#include "gameplay/execution.h"
#include "gameplay/endgame.h"
typedef GameState State;
typedef MoveList List;
typedef Move M;
#define gen generateLegalMoves
#define square createPosition
#define piece(...) createPiece(__VA_ARGS__)
#define place setPiece
#define apply(s, m) applyMove(s, m)
static void init(State *s) {
    GameConfig c;
    initDefaultGameConfig(&c);
    initGameState(s, &c);
}
static int result(State *s) {
    detectGameResult(s);
    return s->result;
}
#else
#include "anteater/rules.h"
typedef AcPosition State;
typedef AcMoveList List;
typedef AcMove M;
#define gen ac_generate_legal_moves
#define square ac_create_position
#define piece(...) ac_create_piece(__VA_ARGS__)
#define place ac_set_piece
static int apply(State *s, M m) {
    AcUndo u;
    return ac_position_apply(s, m, &u);
}
static void init(State *s) {
    ac_position_init(s);
}
static int result(State *s) {
    AcGameResult r;
    assert(!ac_position_result(s, &r));
    return r;
}
#endif
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
        values[n++] = m.specialType;
        values[n++] = m.captureCount;
        values[n++] = m.pathLength;
        for (int k = 0; k < m.captureCount; ++k) {
            values[n++] = m.captures[k].pos.row;
            values[n++] = m.captures[k].pos.col;
            values[n++] = m.captures[k].piece.type;
            values[n++] = m.captures[k].piece.color;
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
    List *l = malloc(sizeof(*l));
    State *next = malloc(sizeof(*next));
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
    State *s = malloc(sizeof(*s));
    List *l = malloc(sizeof(*l));
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
        s->hash = ac_position_hash(s);
#endif
        assert(!gen(s, l));
        fprintf(out, "fixture %d %d %llu %d\n", scenario, l->count, (unsigned long long)fingerprint(l), result(s));
    }
    free(s);
    free(l);
}
