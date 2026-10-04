#ifndef AC_AI_PRIVATE_MOBILITY_H
#define AC_AI_PRIVATE_MOBILITY_H
#include "anteater/types.h"
int ac_ai_count_king_mobility(const AcBoard *board, AcSquare from, AcPiece piece);
int ac_ai_piece_mobility(const AcBoard *board, AcSquare from, AcPiece piece);
#endif
