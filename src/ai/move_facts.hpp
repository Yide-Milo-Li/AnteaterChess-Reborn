#ifndef AC_AI_PRIVATE_MOVE_FACTS_H
#define AC_AI_PRIVATE_MOVE_FACTS_H
#include "anteater/types.hpp"

namespace ac {
int ai_is_promotion_move(const Move *move);
int ai_is_noisy_move(const Move *move);
int ai_is_quiet_move(const Move *move);
int ai_square_index(Square pos);
int ai_absolute_value(int value);
int ai_is_irreversible_move(const Move *move);

} // namespace ac
#endif
