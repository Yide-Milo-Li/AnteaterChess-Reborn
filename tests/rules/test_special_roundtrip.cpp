#include "anteater/rules.hpp"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

using namespace ac;
static void put(Position *p, int r, int c, PieceType type, Color color) {
    set_piece(&p->board, create_position(r, c), create_piece(type, color));
}
int main(void) {
    MoveList *list = static_cast<MoveList *>(malloc(sizeof(*list)));
    assert(list);
    for (int side = 0; side < 2; ++side)
        for (int fixture = 0; fixture < 3; ++fixture) {
            Color color = (Color)side, enemy = color == Color::White ? Color::Black : Color::White;
            int home = color == Color::White ? 7 : 0, far = 7 - home, advance = color == Color::White ? -1 : 1;
            Position p;
            position_init(&p);
            p.currentTurn = color;
            for (int r = 0; r < Rows; ++r)
                for (int c = 0; c < Columns; ++c)
                    remove_piece(&p.board, create_position(r, c));
            put(&p, home, 5, PieceType::King, color);
            put(&p, far, 5, PieceType::King, enemy);
            if (fixture == 0) {
                put(&p, home, 0, PieceType::Rook, color);
                put(&p, home, 9, PieceType::Rook, color);
            }
            if (fixture == 1) {
                put(&p, far - advance, 2, PieceType::Ant, color);
                put(&p, far, 3, PieceType::Rook, enemy);
            }
            if (fixture == 2) {
                int row = color == Color::White ? 3 : 4;
                put(&p, row, 2, PieceType::Ant, color);
                put(&p, row, 3, PieceType::Ant, enemy);
                p.enPassant = create_position(row, 3);
            }
            p.hash = position_hash(&p);
            Position saved = p;
            assert(!generate_legal_moves(&p, list));
            int specials = 0;
            for (int i = 0; i < list->count; ++i) {
                Move move = list->moves[i];
                Undo undo;
                if (move.specialType != SpecialMove::None)
                    ++specials;
                assert(!position_apply(&p, move, &undo));
                assert(p.hash == position_hash(&p));
                assert(!position_unmake(&p, &undo));
                assert((p == saved));
            }
            assert(specials == (fixture == 0 ? 2 : fixture == 1 ? 8 : 1));
        }
    free(list);
    return 0;
}
