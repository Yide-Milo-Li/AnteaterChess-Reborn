#include "anteater/ai.h"
#include <assert.h>
#include <string.h>
static int64_t now(void *data) {
    return (*(int64_t *)data)++;
}
static void empty(AcPosition *p) {
    ac_position_init(p);
    for (int r = 0; r < AC_ROWS; ++r)
        for (int c = 0; c < AC_COLS; ++c)
            ac_remove_piece(&p->board, ac_create_position(r, c));
}
static void put(AcPosition *p, int r, int c, AcPieceType type, AcColor color) {
    ac_set_piece(&p->board, ac_create_position(r, c), ac_create_piece(type, color));
}
int main(void) {
    AcSearchContext *context = ac_search_create();
    assert(context);
    int64_t ms = 0;
    AcSearchOptions o = {
        {now, &ms},
        500, 3, NULL, NULL, NULL, 0
    };
    AcSearchResult result;
    AcPosition p, before;
    AcUndo undo;
    empty(&p);
    put(&p, 7, 5, AC_KING, AC_WHITE);
    put(&p, 7, 0, AC_ROOK, AC_WHITE);
    put(&p, 6, 4, AC_ANT, AC_WHITE);
    put(&p, 5, 5, AC_ROOK, AC_BLACK);
    put(&p, 0, 0, AC_KING, AC_BLACK);
    p.hash = ac_position_hash(&p);
    before = p;
    assert(ac_is_in_check(&p, AC_WHITE));
    assert(!ac_search(context, &p, &o, &result));
    assert(!memcmp(&p, &before, sizeof(p)));
    assert(!ac_position_apply(&p, result.move, &undo));
    assert(!ac_is_in_check(&p, AC_WHITE));
    empty(&p);
    p.currentTurn = AC_BLACK;
    put(&p, 0, 0, AC_KING, AC_BLACK);
    put(&p, 1, 1, AC_QUEEN, AC_WHITE);
    put(&p, 2, 2, AC_KING, AC_WHITE);
    p.hash = ac_position_hash(&p);
    assert(ac_search(context, &p, &o, &result) == AC_UNAVAILABLE);
    empty(&p);
    put(&p, 1, 2, AC_ANT, AC_WHITE);
    put(&p, 0, 2, AC_BISHOP, AC_BLACK);
    put(&p, 0, 3, AC_ROOK, AC_BLACK);
    p.hash = ac_position_hash(&p);
    assert(!ac_search(context, &p, &o, &result));
    assert(ac_is_promotion_special_move(result.move.specialType));
    assert(!ac_position_apply(&p, result.move, &undo));
    empty(&p);
    put(&p, 3, 4, AC_ANT, AC_WHITE);
    put(&p, 3, 5, AC_ANT, AC_BLACK);
    put(&p, 2, 4, AC_ROOK, AC_BLACK);
    p.enPassant = ac_create_position(3, 5);
    p.hash = ac_position_hash(&p);
    assert(!ac_search(context, &p, &o, &result));
    assert(result.move.specialType == AC_EN_PASSANT);
    assert(!ac_position_apply(&p, result.move, &undo));
    ac_search_destroy(context);
    return 0;
}
