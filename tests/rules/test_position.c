#include "anteater/rules.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static AcMove resolve(AcPosition *p, const char *from, const char *to) {
    AcMoveRequest r;
    AcMove m;
    assert(!ac_parse_move_request_fields(from, to, AC_PROMOTION_CHOICE_QUEEN, &r));
    assert(!ac_resolve_move_request(p, r, &m));
    return m;
}
static void empty(AcPosition *p) {
    ac_position_init(p);
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 10; ++c)
            ac_remove_piece(&p->board, ac_create_position(r, c));
}
static void place(AcPosition *p, const char *sq, AcPieceType type, AcColor c) {
    ac_set_piece(&p->board, ac_parse_position(sq), ac_create_piece(type, c));
    p->hash = ac_position_hash(p);
}
int main(void) {
    AcPosition p, initial;
    ac_position_init(&p);
    initial = p;
    AcUndo undo[200];
    unsigned rng = 12345;
    AcMoveList *moves = malloc(sizeof(*moves));
    assert(moves);
    int played = 0;
    for (; played < 150; ++played) {
        assert(!ac_generate_legal_moves(&p, moves));
        if (!moves->count)
            break;
        rng = rng * 1664525u + 1013904223u;
        assert(!ac_position_apply(&p, moves->moves[rng % (unsigned)moves->count], &undo[played]));
        assert(p.hash == ac_position_hash(&p));
    }
    while (played > 0) {
        --played;
        assert(!ac_position_unmake(&p, &undo[played]));
        assert(p.hash == ac_position_hash(&p));
    }
    assert(!memcmp(&p, &initial, sizeof(p)));
    AcMove bad = ac_create_move(ac_parse_position("A1"), ac_parse_position("J8"), ac_create_piece(AC_ROOK, AC_WHITE));
    assert(ac_position_apply(&p, bad, &undo[0]) != AC_OK);
    assert(!memcmp(&p, &initial, sizeof(p)));
    empty(&p);
    place(&p, "F1", AC_KING, AC_WHITE);
    place(&p, "F8", AC_KING, AC_BLACK);
    place(&p, "J1", AC_ROOK, AC_WHITE);
    initial = p;
    AcMove castle = resolve(&p, "F1", "H1");
    assert(castle.specialType == AC_CASTLING_KINGSIDE);
    assert(!ac_position_apply(&p, castle, &undo[0]));
    assert(p.castlingRights == 12);
    assert(!ac_position_unmake(&p, &undo[0]));
    assert(!memcmp(&p, &initial, sizeof(p)));
    empty(&p);
    place(&p, "A1", AC_KING, AC_WHITE);
    place(&p, "J8", AC_KING, AC_BLACK);
    place(&p, "C7", AC_ANT, AC_WHITE);
    AcMove promotion = resolve(&p, "C7", "C8");
    assert(promotion.specialType == AC_PROMOTION_QUEEN);
    initial = p;
    assert(!ac_position_apply(&p, promotion, &undo[0]));
    assert(!ac_position_unmake(&p, &undo[0]));
    assert(!memcmp(&p, &initial, sizeof(p)));
    empty(&p);
    place(&p, "A1", AC_KING, AC_WHITE);
    place(&p, "J8", AC_KING, AC_BLACK);
    place(&p, "E5", AC_ANT, AC_WHITE);
    place(&p, "F7", AC_ANT, AC_BLACK);
    p.currentTurn = AC_BLACK;
    p.hash = ac_position_hash(&p);
    AcMove push = resolve(&p, "F7", "F5");
    assert(!ac_position_apply(&p, push, &undo[0]));
    AcMove ep = resolve(&p, "E5", "F6");
    assert(ep.specialType == AC_EN_PASSANT);
    initial = p;
    assert(!ac_position_apply(&p, ep, &undo[1]));
    assert(!ac_position_unmake(&p, &undo[1]));
    assert(!memcmp(&p, &initial, sizeof(p)));
    empty(&p);
    place(&p, "A1", AC_KING, AC_WHITE);
    place(&p, "J8", AC_KING, AC_BLACK);
    place(&p, "D4", AC_ANTEATER, AC_WHITE);
    place(&p, "E4", AC_ANT, AC_BLACK);
    place(&p, "E5", AC_ANT, AC_BLACK);
    AcMoveRequest req;
    AcMove chain;
    assert(!ac_parse_move_request_fields("D4", "E5", AC_PROMOTION_CHOICE_NONE, &req));
    assert(ac_resolve_move_request(&p, req, &chain) != 0);
    assert(!ac_generate_legal_moves_for_position(&p, ac_parse_position("D4"), moves));
    int found = 0;
    for (int i = 0; i < moves->count; ++i)
        if (moves->moves[i].captureCount == 2) {
            initial = p;
            assert(!ac_position_apply(&p, moves->moves[i], &undo[0]));
            assert(!ac_position_unmake(&p, &undo[0]));
            assert(!memcmp(&p, &initial, sizeof(p)));
            found = 1;
        }
    assert(found);
    ac_init_move_list(moves);
    for (int i = 0; i < AC_MAX_MOVES; ++i)
        assert(!ac_add_move(moves, bad));
    assert(ac_add_move(moves, bad) == AC_CAPACITY && moves->status == AC_CAPACITY);
    free(moves);
    return 0;
}
