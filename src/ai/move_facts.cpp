#include "anteater/rules.hpp"
#include "score_constants.hpp"
#include "move_facts.hpp"

namespace ac {

int ai_is_promotion_move(const Move *move) {
    return move != NULL && is_promotion_special_move(move->specialType);
}

int ai_is_noisy_move(const Move *move) {
    return move->captureCount > 0 || move->specialType == SpecialMove::AnteaterCapture || ai_is_promotion_move(move);
}

int ai_is_quiet_move(const Move *move) {
    return !ai_is_noisy_move(move) && move->specialType != SpecialMove::CastlingKingside &&
           move->specialType != SpecialMove::CastlingQueenside;
}

int ai_square_index(Square pos) {
    return pos.row * Columns + pos.col;
}

int ai_absolute_value(int value) {
    if (value < 0) {
        return -value;
    }

    return value;
}

int ai_is_irreversible_move(const Move *move) {
    if (move == NULL) {
        return 0;
    }

    return move->movedPiece.type == PieceType::Ant || move->captureCount > 0 || ai_is_promotion_move(move);
}

} // namespace ac
