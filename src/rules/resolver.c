#include "anteater/rules.h"
#include <stdlib.h>

#include <stddef.h>

static AcSpecialMove special_for_promotion_choice(AcPromotionChoice promotion) {
    switch (promotion) {
    case AC_PROMOTION_CHOICE_ROOK:
        return AC_PROMOTION_ROOK;
    case AC_PROMOTION_CHOICE_BISHOP:
        return AC_PROMOTION_BISHOP;
    case AC_PROMOTION_CHOICE_KNIGHT:
        return AC_PROMOTION_KNIGHT;
    case AC_PROMOTION_CHOICE_NONE:
    case AC_PROMOTION_CHOICE_QUEEN:
    default:
        return AC_PROMOTION_QUEEN;
    }
}

static int candidate_matches_request(AcMove candidate, AcMoveRequest request, AcPiece movingPiece) {
    return ac_position_equal(candidate.from, request.from) && ac_position_equal(candidate.to, request.to) &&
           candidate.movedPiece.type == movingPiece.type && candidate.movedPiece.color == movingPiece.color;
}

static int resolve_with_workspace(const AcPosition *state, AcMoveRequest request, AcMove *resolvedMove,
                                  AcMoveList *candidates) {
    /* Workspace supplied by the public wrapper. */
    AcPiece movingPiece;
    AcMove *singleNonPromotionMove;
    AcMove *selectedPromotionMove;
    AcSpecialMove requestedPromotion;
    int nonPromotionCount;
    int promotionCount;
    int index;

    if (state == NULL || resolvedMove == NULL || !ac_is_valid_position(request.from) ||
        !ac_is_valid_position(request.to) || !ac_is_valid_promotion_choice(request.promotion)) {
        return 1;
    }

    if (ac_validate_selection(state, request.from) != AC_SELECT_VALID) {
        return 1;
    }

    movingPiece = ac_get_piece(&state->board, request.from);
    if (movingPiece.type == AC_EMPTY_PIECE) {
        return 1;
    }

    int status = ac_generate_legal_moves_for_position(state, request.from, candidates);
    if (status != AC_OK) {
        return status;
    }

    requestedPromotion = special_for_promotion_choice(request.promotion);
    singleNonPromotionMove = NULL;
    selectedPromotionMove = NULL;
    nonPromotionCount = 0;
    promotionCount = 0;

    for (index = 0; index < ac_get_move_count(candidates); ++index) {
        AcMove *candidate = ac_get_move(candidates, index);

        if (candidate == NULL || !candidate_matches_request(*candidate, request, movingPiece)) {
            continue;
        }

        if (ac_is_promotion_special_move(candidate->specialType)) {
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

int ac_resolve_move_request(const AcPosition *s, AcMoveRequest r, AcMove *m) {
    AcMoveList *l = malloc(sizeof(*l));
    if (!l)
        return AC_OUT_OF_MEMORY;
    int result = resolve_with_workspace(s, r, m, l);
    free(l);
    return result;
}
