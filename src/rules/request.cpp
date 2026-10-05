#include "anteater/rules.h"

#include <stddef.h>

static void mark_invalid_request(AcMoveRequest *request) {
    if (request == NULL) {
        return;
    }

    request->from = ac_create_position(-1, -1);
    request->to = ac_create_position(-1, -1);
    request->promotion = AC_PROMOTION_CHOICE_NONE;
}

int ac_is_valid_promotion_choice(AcPromotionChoice promotion) {
    return promotion == AC_PROMOTION_CHOICE_NONE || promotion == AC_PROMOTION_CHOICE_QUEEN ||
           promotion == AC_PROMOTION_CHOICE_ROOK || promotion == AC_PROMOTION_CHOICE_BISHOP ||
           promotion == AC_PROMOTION_CHOICE_KNIGHT;
}

int ac_create_move_request(AcMoveRequest *request, AcSquare from, AcSquare to, AcPromotionChoice promotion) {
    if (request == NULL || !ac_is_valid_position(from) || !ac_is_valid_position(to) ||
        !ac_is_valid_promotion_choice(promotion)) {
        mark_invalid_request(request);
        return 1;
    }

    request->from = from;
    request->to = to;
    request->promotion = promotion;
    return 0;
}
