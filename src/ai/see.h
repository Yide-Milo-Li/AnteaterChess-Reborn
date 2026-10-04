#ifndef AC_AI_PRIVATE_SEE_H
#define AC_AI_PRIVATE_SEE_H
#include "anteater/types.h"
int ac_ai_see_initial_gain(const AcMove *move);
int ac_ai_see_move_score(const AcPosition *state, const AcMove *move);
#endif
