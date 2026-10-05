#ifndef AC_AI_PRIVATE_EVALUATION_FEATURES_H
#define AC_AI_PRIVATE_EVALUATION_FEATURES_H
#include "anteater/types.hpp"

namespace ac {
int ai_evaluate_pawns(const Position *state, Color color, const int antFiles[2][Columns], int phase);
int ai_evaluate_rook_and_queen_files(const Position *state, Color color, const int antFiles[2][Columns], int phase);
int ai_evaluate_king_safety(const Position *state, Color color, Square kingPos, int phase);
int ai_evaluate_development(const Position *state, Color color, int phase);
int ai_evaluate_anteater_threats(const Board *board, Color color);
int ai_evaluate_outposts(const Board *board, Color color, int phase);

} // namespace ac
#endif
