#include "input/move_request.h"

#include <stddef.h>

static void mark_invalid_request(MoveRequest *request) {
    if (request == NULL) {
        return;
    }

    request->from = createPosition(-1, -1);
    request->to = createPosition(-1, -1);
    request->promotion = PROMOTION_CHOICE_NONE;
}

int isValidPromotionChoice(PromotionChoice promotion) {
    return promotion == PROMOTION_CHOICE_NONE
        || promotion == PROMOTION_CHOICE_QUEEN
        || promotion == PROMOTION_CHOICE_ROOK
        || promotion == PROMOTION_CHOICE_BISHOP
        || promotion == PROMOTION_CHOICE_KNIGHT;
}

int createMoveRequest(MoveRequest *request, Position from, Position to,
                      PromotionChoice promotion) {
    if (request == NULL || !isValidPosition(from) || !isValidPosition(to)
        || !isValidPromotionChoice(promotion)) {
        mark_invalid_request(request);
        return 1;
    }

    request->from = from;
    request->to = to;
    request->promotion = promotion;
    return 0;
}

int createMoveRequestFromCommand(MoveRequest *request, Command command) {
    if (command.type != CMD_MOVE) {
        mark_invalid_request(request);
        return 1;
    }

    return createMoveRequest(request, command.from, command.to,
        PROMOTION_CHOICE_QUEEN);
}
