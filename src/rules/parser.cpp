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

int ac_parse_move_request_fields(const char *fromText, const char *toText, AcPromotionChoice promotion,
                                 AcMoveRequest *request) {
    AcSquare from;
    AcSquare to;

    if (request == NULL || fromText == NULL || toText == NULL || !ac_is_valid_promotion_choice(promotion)) {
        mark_invalid_request(request);
        return 1;
    }

    from = ac_parse_position(fromText);
    to = ac_parse_position(toText);
    if (!ac_is_valid_position(from) || !ac_is_valid_position(to)) {
        mark_invalid_request(request);
        return 1;
    }

    return ac_create_move_request(request, from, to, promotion);
}
