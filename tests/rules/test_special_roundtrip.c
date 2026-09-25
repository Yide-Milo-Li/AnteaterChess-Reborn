#include "anteater/rules.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static void put(AcPosition *p, int r, int c, AcPieceType type, AcColor color) {
    ac_set_piece(&p->board, ac_create_position(r, c), ac_create_piece(type, color));
}
int main(void) {
    AcMoveList *list = malloc(sizeof(*list));
    assert(list);
    for (int side = 0; side < 2; ++side)
        for (int fixture = 0; fixture < 3; ++fixture) {
            AcColor color = (AcColor)side, enemy = color == AC_WHITE ? AC_BLACK : AC_WHITE;
            int home = color == AC_WHITE ? 7 : 0, far = 7 - home, advance = color == AC_WHITE ? -1 : 1;
            AcPosition p;
            ac_position_init(&p);
            p.currentTurn = color;
            for (int r = 0; r < AC_ROWS; ++r)
                for (int c = 0; c < AC_COLS; ++c)
                    ac_remove_piece(&p.board, ac_create_position(r, c));
            put(&p, home, 5, AC_KING, color);
            put(&p, far, 5, AC_KING, enemy);
            if (fixture == 0) {
                put(&p, home, 0, AC_ROOK, color);
                put(&p, home, 9, AC_ROOK, color);
            }
            if (fixture == 1) {
                put(&p, far - advance, 2, AC_ANT, color);
                put(&p, far, 3, AC_ROOK, enemy);
            }
            if (fixture == 2) {
                int row = color == AC_WHITE ? 3 : 4;
                put(&p, row, 2, AC_ANT, color);
                put(&p, row, 3, AC_ANT, enemy);
                p.enPassant = ac_create_position(row, 3);
            }
            p.hash = ac_position_hash(&p);
            AcPosition saved = p;
            assert(!ac_generate_legal_moves(&p, list));
            int specials = 0;
            for (int i = 0; i < list->count; ++i) {
                AcMove move = list->moves[i];
                AcUndo undo;
                if (move.specialType != AC_NO_SPECIAL_MOVE)
                    ++specials;
                assert(!ac_position_apply(&p, move, &undo));
                assert(p.hash == ac_position_hash(&p));
                assert(!ac_position_unmake(&p, &undo));
                assert(!memcmp(&p, &saved, sizeof(p)));
            }
            assert(specials == (fixture == 0 ? 2 : fixture == 1 ? 8 : 1));
        }
    free(list);
    return 0;
}
