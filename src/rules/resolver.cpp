#include "anteater/rules.hpp"
#include "anteater/memory.hpp"

#include <cmath>
#include <stddef.h>

namespace ac {

static SpecialMove special_for_promotion_choice(PromotionChoice promotion) {
    switch (promotion) {
    case PromotionChoice::Rook:
        return SpecialMove::PromotionRook;
    case PromotionChoice::Bishop:
        return SpecialMove::PromotionBishop;
    case PromotionChoice::Knight:
        return SpecialMove::PromotionKnight;
    case PromotionChoice::None:
    case PromotionChoice::Queen:
    default:
        return SpecialMove::PromotionQueen;
    }
}

static int candidate_matches_request(Move candidate, MoveRequest request, Piece movingPiece) {
    return position_equal(candidate.from, request.from) && position_equal(candidate.to, request.to) &&
           candidate.movedPiece.type == movingPiece.type && candidate.movedPiece.color == movingPiece.color;
}

static Status resolve_with_workspace(const Position *state, MoveRequest request, Move *resolvedMove,
                                     MoveList *candidates) {
    /* Workspace supplied by the public wrapper. */
    Piece movingPiece;
    Move *singleNonPromotionMove;
    Move *selectedPromotionMove;
    SpecialMove requestedPromotion;
    int nonPromotionCount;
    int promotionCount;
    int index;

    if (state == NULL || resolvedMove == NULL || !is_valid_position(request.from) || !is_valid_position(request.to) ||
        !is_valid_promotion_choice(request.promotion)) {
        return Status::InvalidArgument;
    }

    if (validate_selection(state, request.from) != SelectionResult::Valid) {
        return Status::InvalidArgument;
    }

    movingPiece = get_piece(&state->board, request.from);
    if (movingPiece.type == PieceType::Empty) {
        return Status::InvalidArgument;
    }

    Status status = generate_legal_moves_for_position(state, request.from, candidates);
    if (status != Status::Ok) {
        return status;
    }

    requestedPromotion = special_for_promotion_choice(request.promotion);
    singleNonPromotionMove = NULL;
    selectedPromotionMove = NULL;
    nonPromotionCount = 0;
    promotionCount = 0;

    for (index = 0; index < get_move_count(candidates); ++index) {
        Move *candidate = get_move(candidates, index);

        if (candidate == NULL || !candidate_matches_request(*candidate, request, movingPiece)) {
            continue;
        }

        if (is_promotion_special_move(candidate->specialType)) {
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
            return Status::InvalidArgument;
        }

        *resolvedMove = *selectedPromotionMove;
        return Status::Ok;
    }

    if (nonPromotionCount == 1 && singleNonPromotionMove != NULL) {
        *resolvedMove = *singleNonPromotionMove;
        return Status::Ok;
    }

    if (nonPromotionCount > 1 && movingPiece.type == PieceType::Anteater) {
        bool isAdjacent = (std::abs(request.from.row - request.to.row) <= 1 &&
                           std::abs(request.from.col - request.to.col) <= 1);
        Move *bestCandidate = nullptr;

        if (isAdjacent) {
            // Adjacent target: prioritize direct 1-step capture (captureCount == 1)
            for (index = 0; index < get_move_count(candidates); ++index) {
                Move *candidate = get_move(candidates, index);
                if (candidate == NULL || !candidate_matches_request(*candidate, request, movingPiece)) {
                    continue;
                }
                if (is_promotion_special_move(candidate->specialType)) {
                    continue;
                }
                if (candidate->captureCount == 1) {
                    bestCandidate = candidate;
                    break;
                }
            }
        }

        // If not adjacent, or if no direct 1-step capture exists:
        // Prioritize greedy maximal captures (highest captureCount)
        if (bestCandidate == nullptr) {
            int maxCaptures = -1;
            for (index = 0; index < get_move_count(candidates); ++index) {
                Move *candidate = get_move(candidates, index);
                if (candidate == NULL || !candidate_matches_request(*candidate, request, movingPiece)) {
                    continue;
                }
                if (is_promotion_special_move(candidate->specialType)) {
                    continue;
                }
                if (candidate->captureCount > maxCaptures) {
                    maxCaptures = candidate->captureCount;
                    bestCandidate = candidate;
                }
            }
        }

        if (bestCandidate != nullptr) {
            *resolvedMove = *bestCandidate;
            return Status::Ok;
        }
    }

    return Status::InvalidArgument;
}

Status resolve_move_request(const Position *s, MoveRequest r, Move *m, std::pmr::memory_resource *resource) {
    if (!resource)
        return Status::InvalidArgument;
    try {
        auto workspace = detail::make_owned<MoveList>(resource);
        MoveList *l = workspace.get();
        Status result = resolve_with_workspace(s, r, m, l);
        return result;

    } catch (const std::bad_alloc &) {
        return Status::OutOfMemory;
    }
}
} // namespace ac
