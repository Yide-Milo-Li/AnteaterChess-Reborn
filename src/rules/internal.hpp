#ifndef AC_RULES_INTERNAL_H
#define AC_RULES_INTERNAL_H
#include "anteater/rules.hpp"

namespace ac {
/* Only generated moves may use this unchecked primitive. */
Status position_make(Position *position, Move move, Undo *undo);
int square_attacked(const Board *board, Square square, Color by);

} // namespace ac
#endif
