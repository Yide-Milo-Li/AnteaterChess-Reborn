#ifndef AC_AI_PRIVATE_EVALUATION_FEATURES_H
#define AC_AI_PRIVATE_EVALUATION_FEATURES_H
#include "anteater/types.h"
int ac_ai_evaluate_pawns(const AcPosition *state, AcColor color, const int antFiles[2][AC_COLS], int phase);
int ac_ai_evaluate_rook_and_queen_files(const AcPosition *state, AcColor color, const int antFiles[2][AC_COLS],
                                        int phase);
int ac_ai_evaluate_king_safety(const AcPosition *state, AcColor color, AcSquare kingPos, int phase);
int ac_ai_evaluate_development(const AcPosition *state, AcColor color, int phase);
int ac_ai_evaluate_anteater_threats(const AcBoard *board, AcColor color);
int ac_ai_evaluate_outposts(const AcBoard *board, AcColor color, int phase);
#endif
