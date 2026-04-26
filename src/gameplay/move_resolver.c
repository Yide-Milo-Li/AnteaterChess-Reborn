#include "gameplay/move_resolver.h"

#include <stddef.h>

#include "core/board.h"
#include "core/move.h"
#include "core/movelist.h"
#include "gameplay/movegen.h"
#include "gameplay/validation.h"

static SpecialMove special_for_promotion_choice(PromotionChoice promotion) {
    switch (promotion) {
        case PROMOTION_CHOICE_ROOK:
            return PROMOTION_ROOK;
        case PROMOTION_CHOICE_BISHOP:
            return PROMOTION_BISHOP;
        case PROMOTION_CHOICE_KNIGHT:
            return PROMOTION_KNIGHT;
        case PROMOTION_CHOICE_NONE:
        case PROMOTION_CHOICE_QUEEN:
        default:
            return PROMOTION_QUEEN;
    }
}

static int candidate_matches_request(Move candidate, MoveRequest request,
                                     Piece movingPiece) {
    return positionEqual(candidate.from, request.from)
        && positionEqual(candidate.to, request.to)
        && candidate.movedPiece.type == movingPiece.type
        && candidate.movedPiece.color == movingPiece.color;
}

int resolveMoveRequest(const GameState *state, MoveRequest request,
                       Move *resolvedMove) {
    MoveList candidates;
    Piece movingPiece;
    Move *singleNonPromotionMove;
    Move *selectedPromotionMove;
    SpecialMove requestedPromotion;
    int nonPromotionCount;
    int promotionCount;
    int index;

    if (state == NULL || resolvedMove == NULL
        || !isValidPosition(request.from) || !isValidPosition(request.to)
        || !isValidPromotionChoice(request.promotion)) {
        return 1;
    }

    if (validateSelection(state, request.from) != SELECT_VALID) {
        return 1;
    }

    movingPiece = getPiece(&state->board, request.from);
    if (movingPiece.type == EMPTY_PIECE) {
        return 1;
    }

    if (generateLegalMovesForPosition(state, request.from, &candidates) != 0) {
        return 1;
    }

    requestedPromotion = special_for_promotion_choice(request.promotion);
    singleNonPromotionMove = NULL;
    selectedPromotionMove = NULL;
    nonPromotionCount = 0;
    promotionCount = 0;

    for (index = 0; index < getMoveCount(&candidates); ++index) {
        Move *candidate = getMove(&candidates, index);

        if (candidate == NULL
            || !candidate_matches_request(*candidate, request, movingPiece)) {
            continue;
        }

        if (isPromotionSpecialMove(candidate->specialType)) {
            ++promotionCount;
            if (candidate->specialType == requestedPromotion) {
                selectedPromotionMove = candidate;
            }
        } else {
            ++nonPromotionCount;
            if (nonPromotionCount == 1) {
                singleNonPromotionMove = candidate;
            }
        }
    }

    if (promotionCount > 0) {
        if (selectedPromotionMove == NULL) {
            return 1;
        }

        *resolvedMove = *selectedPromotionMove;
        return 0;
    }

    if (nonPromotionCount == 1 && singleNonPromotionMove != NULL) {
        *resolvedMove = *singleNonPromotionMove;
        return 0;
    }

    return 1;
}
