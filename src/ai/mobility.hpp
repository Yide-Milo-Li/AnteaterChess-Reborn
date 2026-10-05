#ifndef AC_AI_PRIVATE_MOBILITY_H
#define AC_AI_PRIVATE_MOBILITY_H
#include "anteater/types.hpp"

namespace ac {
int ai_count_king_mobility(const Board *board, Square from, Piece piece);
int ai_piece_mobility(const Board *board, Square from, Piece piece);

} // namespace ac
#endif
