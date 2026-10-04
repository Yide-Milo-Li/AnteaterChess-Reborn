#ifndef AC_AI_PRIVATE_MOVE_FACTS_H
#define AC_AI_PRIVATE_MOVE_FACTS_H
#include "anteater/types.h"
int ac_ai_is_promotion_move(const AcMove *move);
int ac_ai_is_noisy_move(const AcMove *move);
int ac_ai_is_quiet_move(const AcMove *move);
int ac_ai_square_index(AcSquare pos);
int ac_ai_absolute_value(int value);
int ac_ai_is_irreversible_move(const AcMove *move);
#endif
