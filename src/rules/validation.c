#include "anteater/rules.h"
#include <stdlib.h>

#include <stddef.h>

static int moves_match_exactly(AcMove expected, AcMove candidate) {
    int index;

    if (!ac_position_equal(expected.from, candidate.from) || !ac_position_equal(expected.to, candidate.to)) {
        return 0;
    }

    if (expected.movedPiece.type != candidate.movedPiece.type ||
        expected.movedPiece.color != candidate.movedPiece.color || expected.specialType != candidate.specialType ||
        expected.captureCount != candidate.captureCount || expected.pathLength != candidate.pathLength) {
        return 0;
    }

    for (index = 0; index < expected.pathLength; ++index) {
        if (!ac_position_equal(expected.path[index], candidate.path[index])) {
            return 0;
        }
    }

    for (index = 0; index < expected.captureCount; ++index) {
        if (!ac_position_equal(expected.captures[index].pos, candidate.captures[index].pos) ||
            expected.captures[index].piece.type != candidate.captures[index].piece.type ||
            expected.captures[index].piece.color != candidate.captures[index].piece.color) {
            return 0;
        }
    }

    return 1;
}

static int move_requests_explicit_special_semantics(AcMove move) {
    return move.specialType != AC_NO_SPECIAL_MOVE || move.captureCount > 0 || move.pathLength > 0;
}

/* Callers sometimes only know from/to before special-move metadata is derived.
 * In that common case, matching the destination against generated candidates is
 * enough; detailed requests still require an exact semantic match. */
static int generated_move_matches_request(AcMove requested, AcMove candidate) {
    if (!ac_position_equal(requested.from, candidate.from) || !ac_position_equal(requested.to, candidate.to)) {
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

AcSelectionResult ac_validate_selection(const AcPosition *state, AcSquare pos) {
    AcPiece piece;

    if (state == NULL || !ac_is_valid_position(pos)) {
        return AC_SELECT_OUT_OF_BOUNDS;
    }

    piece = ac_get_piece(&state->board, pos);
    if (piece.type == AC_EMPTY_PIECE) {
        return AC_SELECT_EMPTY;
    }

    if (piece.color != state->currentTurn) {
        return AC_SELECT_OPPONENT_PIECE;
    }

    return AC_SELECT_VALID;
}

static int resolve_with_workspace(const AcPosition *state, AcMove move, AcMoveList *candidates) {
    /* Workspace supplied by the public wrapper. */
    int index;
    int simpleMatchCount;
    int allSimpleMatchesArePromotions;
    int queenVariantFound;

    if (state == NULL || !ac_is_valid_position(move.from) || !ac_is_valid_position(move.to)) {
        return 0;
    }

    if (ac_generate_legal_moves_for_position(state, move.from, candidates) != 0) {
        return 0;
    }

    simpleMatchCount = 0;
    allSimpleMatchesArePromotions = 1;
    queenVariantFound = 0;
    for (index = 0; index < ac_get_move_count(candidates); ++index) {
        AcMove *candidate = ac_get_move(candidates, index);

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
            if (!ac_is_promotion_special_move(candidate->specialType)) {
                allSimpleMatchesArePromotions = 0;
            }
            if (candidate->specialType == AC_PROMOTION_QUEEN) {
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

int ac_validate_move(const AcPosition *s, AcMove m) {
    AcMoveList *l = malloc(sizeof(*l));
    if (!l)
        return 0;
    int result = resolve_with_workspace(s, m, l);
    free(l);
    return result;
}
