#ifndef AC_AI_PRIVATE_PIECE_TABLES_H
#define AC_AI_PRIVATE_PIECE_TABLES_H
#include "anteater/types.h"
int ac_ai_piece_value(AcPieceType type);
int ac_ai_phase_value(AcPieceType type);
int ac_ai_mobility_weight(AcPieceType type);
int ac_ai_pst_bonus(AcPiece piece, int row, int col, int phase);
#endif
