#ifndef AC_AI_PRIVATE_PIECE_TABLES_H
#define AC_AI_PRIVATE_PIECE_TABLES_H
#include "anteater/types.hpp"

namespace ac {
int ai_piece_value(PieceType type);
int ai_phase_value(PieceType type);
int ai_mobility_weight(PieceType type);
int ai_pst_bonus(Piece piece, int row, int col, int phase);

} // namespace ac
#endif
