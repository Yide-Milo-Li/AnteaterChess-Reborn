#ifndef AC_AI_PRIVATE_EVALUATION_H
#define AC_AI_PRIVATE_EVALUATION_H
#include "anteater/types.hpp"

namespace ac {
int ai_evaluate_absolute(const Position *state);
int ai_evaluate_relative(const Position *state);

} // namespace ac
#endif
