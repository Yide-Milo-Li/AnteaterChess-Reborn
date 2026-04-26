#include "input/move_request_parser.h"

#include <stddef.h>

#include "core/position.h"

static void mark_invalid_request(MoveRequest *request) {
    if (request == NULL) {
        return;
    }

    request->from = createPosition(-1, -1);
    request->to = createPosition(-1, -1);
    request->promotion = PROMOTION_CHOICE_NONE;
}

int parseMoveRequestFields(const char *fromText,
                           const char *toText,
                           PromotionChoice promotion,
                           MoveRequest *request) {
    Position from;
    Position to;

    if (request == NULL || fromText == NULL || toText == NULL
        || !isValidPromotionChoice(promotion)) {
        mark_invalid_request(request);
        return 1;
    }

    from = parsePosition(fromText);
    to = parsePosition(toText);
    if (!isValidPosition(from) || !isValidPosition(to)) {
        mark_invalid_request(request);
        return 1;
    }

    return createMoveRequest(request, from, to, promotion);
}
