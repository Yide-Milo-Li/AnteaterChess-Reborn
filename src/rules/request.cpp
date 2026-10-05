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

int is_valid_promotion_choice(PromotionChoice promotion) {
    return promotion == PromotionChoice::None || promotion == PromotionChoice::Queen ||
           promotion == PromotionChoice::Rook || promotion == PromotionChoice::Bishop ||
           promotion == PromotionChoice::Knight;
}

Status create_move_request(MoveRequest *request, Square from, Square to, PromotionChoice promotion) {
    if (request == NULL || !is_valid_position(from) || !is_valid_position(to) ||
        !is_valid_promotion_choice(promotion)) {
        mark_invalid_request(request);
        return Status::InvalidArgument;
    }

    request->from = from;
    request->to = to;
    request->promotion = promotion;
    return Status::Ok;
}

} // namespace ac
