#ifndef AC_AI_PRIVATE_SEE_H
#define AC_AI_PRIVATE_SEE_H
#include "anteater/types.hpp"

namespace ac {
int ai_see_initial_gain(const Move *move);
int ai_see_move_score(const Position *state, const Move *move);

} // namespace ac
#endif
