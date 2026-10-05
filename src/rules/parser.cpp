#include "anteater/rules.hpp"

#include <stddef.h>

namespace ac {

static void mark_invalid_request(MoveRequest *request) {
    if (request == NULL) {
        return;
    }

    request->from = create_position(-1, -1);
    request->to = create_position(-1, -1);
    request->promotion = PromotionChoice::None;
}

Status parse_move_request_fields(const char *fromText, const char *toText, PromotionChoice promotion,
                                 MoveRequest *request) {
    Square from;
    Square to;

    if (request == NULL || fromText == NULL || toText == NULL || !is_valid_promotion_choice(promotion)) {
        mark_invalid_request(request);
        return Status::InvalidArgument;
    }

    from = parse_position(fromText);
    to = parse_position(toText);
    if (!is_valid_position(from) || !is_valid_position(to)) {
        mark_invalid_request(request);
        return Status::InvalidArgument;
    }

    return create_move_request(request, from, to, promotion);
}

} // namespace ac
