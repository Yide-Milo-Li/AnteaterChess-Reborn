#ifndef AC_RULES_INTERNAL_H
#define AC_RULES_INTERNAL_H
#include "anteater/rules.h"
/* Only generated moves may use this unchecked primitive. */
AcStatus ac_position_make(AcPosition *position, AcMove move, AcUndo *undo);
int ac_square_attacked(const AcBoard *board, AcSquare square, AcColor by);
#endif
