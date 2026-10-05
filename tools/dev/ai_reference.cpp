/* Captures independent deterministic behavior before evaluation extraction. */
#include "anteater/ai.h"
#include "evaluation.h"
#include "see.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t fold(uint64_t hash, int value) {
    return (hash ^ (uint32_t)value) * UINT64_C(1099511628211);
}
static uint64_t signature(const AcMove *m) {
    uint64_t h = UINT64_C(1469598103934665603);
    h = fold(h, m->from.row); h = fold(h, m->from.col);
    h = fold(h, m->to.row); h = fold(h, m->to.col);
    h = fold(h, m->specialType); h = fold(h, m->pathLength);
    for (int i = 0; i < m->pathLength; ++i) {
        h = fold(h, m->path[i].row); h = fold(h, m->path[i].col);
    }
    h = fold(h, m->captureCount);
    for (int i = 0; i < m->captureCount; ++i) {
        h = fold(h, m->captures[i].pos.row); h = fold(h, m->captures[i].pos.col);
        h = fold(h, m->captures[i].piece.type); h = fold(h, m->captures[i].piece.color);
    }
    return h;
}
static int64_t now(void *unused) { (void)unused; return 0; }
static void emit(const AcPosition *p, int index) {
    AcMoveList *list = static_cast<AcMoveList *>(malloc(sizeof(*list)));
    if (!list || ac_generate_legal_moves(p, list)) exit(1);
    uint64_t see = UINT64_C(1469598103934665603);
    for (int i = 0; i < list->count; ++i) {
        see ^= signature(&list->moves[i]);
        see = fold(see, ac_ai_see_move_score(p, &list->moves[i]));
    }
    AcSearchContext *ctx = ac_search_create();
    AcSearchOptions o = {{now, NULL}, 10000, 2, NULL, NULL, NULL, 0};
    AcSearchResult r = {};
    AcStatus s = ac_search(ctx, p, &o, &r);
    printf("%d hash=%" PRIu64 " absolute=%d relative=%d moves=%d see=%" PRIu64
           " status=%d depth=%d nodes=%d move=%" PRIu64 "\n", index, p->hash,
           ac_ai_evaluate_absolute(p), ac_ai_evaluate_relative(p), list->count,
           see, s, r.completedDepth, r.nodes, s == AC_OK ? signature(&r.move) : 0);
    ac_search_destroy(ctx);
    free(list);
}
static void clear(AcPosition *p) {
    ac_position_init(p);
    for (int r = 0; r < AC_ROWS; ++r)
        for (int c = 0; c < AC_COLS; ++c) ac_remove_piece(&p->board, AcSquare{r,c});
}
static void put(AcPosition *p, int r, int c, AcPieceType t, AcColor color) {
    ac_set_piece(&p->board, AcSquare{r,c}, ac_create_piece(t, color));
}
int main(void) {
    AcPosition p;
    AcMoveList *moves = static_cast<AcMoveList *>(malloc(sizeof(*moves)));
    uint32_t seed = 0x41c0ffee;
    ac_position_init(&p);
    for (int i = 0; i < 24; ++i) {
        emit(&p, i);
        for (int k = 0; k < 3; ++k) {
            if (ac_generate_legal_moves(&p, moves) || !moves->count) break;
            seed = seed * 1664525u + 1013904223u;
            AcUndo undo;
            if (ac_position_apply(&p, moves->moves[seed % moves->count], &undo)) return 1;
        }
    }
    clear(&p);
    put(&p,7,5,AC_KING,AC_WHITE); put(&p,0,5,AC_KING,AC_BLACK);
    put(&p,3,3,AC_ANTEATER,AC_WHITE); put(&p,2,2,AC_ANT,AC_BLACK);
    put(&p,2,3,AC_ANT,AC_BLACK); put(&p,1,3,AC_ANT,AC_BLACK);
    p.hash = ac_position_hash(&p); emit(&p,24);
    clear(&p);
    put(&p,7,5,AC_KING,AC_WHITE); put(&p,0,5,AC_KING,AC_BLACK);
    put(&p,1,2,AC_ANT,AC_WHITE); put(&p,0,3,AC_ROOK,AC_BLACK);
    p.hash = ac_position_hash(&p); emit(&p,25);
    clear(&p);
    put(&p,7,5,AC_KING,AC_WHITE); put(&p,0,5,AC_KING,AC_BLACK);
    put(&p,3,4,AC_ANT,AC_WHITE); put(&p,3,5,AC_ANT,AC_BLACK);
    p.enPassant = AcSquare{3,5}; p.hash = ac_position_hash(&p); emit(&p,26);
    free(moves);
    return 0;
}
