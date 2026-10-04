#include "anteater/rules.h"
#include "score_constants.h"
#include "move_facts.h"

int ac_ai_is_promotion_move(const AcMove *move) {
    return move != NULL && ac_is_promotion_special_move(move->specialType);
}

int ac_ai_is_noisy_move(const AcMove *move) {
    return move->captureCount > 0 || move->specialType == AC_ANTEATER_CAPTURE || ac_ai_is_promotion_move(move);
}

int ac_ai_is_quiet_move(const AcMove *move) {
    return !ac_ai_is_noisy_move(move) && move->specialType != AC_CASTLING_KINGSIDE &&
           move->specialType != AC_CASTLING_QUEENSIDE;
}

int ac_ai_square_index(AcSquare pos) {
    return pos.row * AC_COLS + pos.col;
}

int ac_ai_absolute_value(int value) {
    if (value < 0) {
        return -value;
    }

    return value;
}

int ac_ai_is_irreversible_move(const AcMove *move) {
    if (move == NULL) {
        return 0;
    }

    return move->movedPiece.type == AC_ANT || move->captureCount > 0 || ac_ai_is_promotion_move(move);
}

