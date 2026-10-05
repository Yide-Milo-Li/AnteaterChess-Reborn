#include "anteater/rules.hpp"
#include "anteater/memory.hpp"

#include <stddef.h>

namespace ac {

static int moves_match_exactly(Move expected, Move candidate) {
    int index;

    if (!position_equal(expected.from, candidate.from) || !position_equal(expected.to, candidate.to)) {
        return 0;
    }

    if (expected.movedPiece.type != candidate.movedPiece.type ||
        expected.movedPiece.color != candidate.movedPiece.color || expected.specialType != candidate.specialType ||
        expected.captureCount != candidate.captureCount || expected.pathLength != candidate.pathLength) {
        return 0;
    }

    for (index = 0; index < expected.pathLength; ++index) {
        if (!position_equal(expected.path[index], candidate.path[index])) {
            return 0;
        }
    }

    for (index = 0; index < expected.captureCount; ++index) {
        if (!position_equal(expected.captures[index].pos, candidate.captures[index].pos) ||
            expected.captures[index].piece.type != candidate.captures[index].piece.type ||
            expected.captures[index].piece.color != candidate.captures[index].piece.color) {
            return 0;
        }
    }

    return 1;
}

static int move_requests_explicit_special_semantics(Move move) {
    return move.specialType != SpecialMove::None || move.captureCount > 0 || move.pathLength > 0;
}

/* Callers sometimes only know from/to before special-move metadata is derived.
 * In that common case, matching the destination against generated candidates is
 * enough; detailed requests still require an exact semantic match. */
static int generated_move_matches_request(Move requested, Move candidate) {
    if (!position_equal(requested.from, candidate.from) || !position_equal(requested.to, candidate.to)) {
        return 0;
    }

    if (requested.movedPiece.type != candidate.movedPiece.type ||
        requested.movedPiece.color != candidate.movedPiece.color) {
        return 0;
    }

    if (move_requests_explicit_special_semantics(requested)) {
        return moves_match_exactly(requested, candidate);
    }

    return 1;
}

SelectionResult validate_selection(const Position *state, Square pos) {
    Piece piece;

    if (state == NULL || !is_valid_position(pos)) {
        return SelectionResult::OutOfBounds;
    }

    piece = get_piece(&state->board, pos);
    if (piece.type == PieceType::Empty) {
        return SelectionResult::Empty;
    }

    if (piece.color != state->currentTurn) {
        return SelectionResult::OpponentPiece;
    }

    return SelectionResult::Valid;
}

static int resolve_with_workspace(const Position *state, Move move, MoveList *candidates) {
    /* Workspace supplied by the public wrapper. */
    int index;
    int simpleMatchCount;
    int allSimpleMatchesArePromotions;
    int queenVariantFound;

    if (state == NULL || !is_valid_position(move.from) || !is_valid_position(move.to)) {
        return 0;
    }

    if (generate_legal_moves_for_position(state, move.from, candidates) != Status::Ok) {
        return 0;
    }

    simpleMatchCount = 0;
    allSimpleMatchesArePromotions = 1;
    queenVariantFound = 0;
    for (index = 0; index < get_move_count(candidates); ++index) {
        Move *candidate = get_move(candidates, index);

        if (candidate == NULL) {
            continue;
        }

        if (move_requests_explicit_special_semantics(move)) {
            if (generated_move_matches_request(move, *candidate)) {
                return 1;
            }
            continue;
        }

        if (generated_move_matches_request(move, *candidate)) {
            ++simpleMatchCount;
            if (!is_promotion_special_move(candidate->specialType)) {
                allSimpleMatchesArePromotions = 0;
            }
            if (candidate->specialType == SpecialMove::PromotionQueen) {
                queenVariantFound = 1;
            }
        }
    }

    if (simpleMatchCount == 1) {
        return 1;
    }

    if (simpleMatchCount > 1 && allSimpleMatchesArePromotions && queenVariantFound) {
        return 1;
    }

    return 0;
}

int validate_move(const Position *s, Move m, std::pmr::memory_resource *resource) {
    if (!resource)
        return 0;
    try {
        auto workspace = detail::make_owned<MoveList>(resource);
        MoveList *l = workspace.get();
        int result = resolve_with_workspace(s, m, l);
        return result;

    } catch (const std::bad_alloc &) {
        return 0;
    }
}
} // namespace ac
