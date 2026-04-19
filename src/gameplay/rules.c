#include "gameplay/validation.h"

#include "gameplay/movegen.h"

static int moves_match_exactly(Move expected, Move candidate) {
    int index;

    if (!positionEqual(expected.from, candidate.from) || !positionEqual(expected.to, candidate.to)) {
        return 0;
    }

    if (expected.movedPiece.type != candidate.movedPiece.type
        || expected.movedPiece.color != candidate.movedPiece.color
        || expected.specialType != candidate.specialType
        || expected.captureCount != candidate.captureCount
        || expected.pathLength != candidate.pathLength) {
        return 0;
    }

    for (index = 0; index < expected.pathLength; ++index) {
        if (!positionEqual(expected.path[index], candidate.path[index])) {
            return 0;
        }
    }

    for (index = 0; index < expected.captureCount; ++index) {
        if (!positionEqual(expected.captures[index].pos, candidate.captures[index].pos)
            || expected.captures[index].piece.type != candidate.captures[index].piece.type
            || expected.captures[index].piece.color != candidate.captures[index].piece.color) {
            return 0;
        }
    }

    return 1;
}

static int move_requests_explicit_special_semantics(Move move) {
    return move.specialType != NO_SPECIAL_MOVE || move.captureCount > 0 || move.pathLength > 0;
}

/* Callers sometimes only know from/to before special-move metadata is derived.
 * In that common case, matching the destination against generated candidates is
 * enough; detailed requests still require an exact semantic match. */
static int generated_move_matches_request(Move requested, Move candidate) {
    if (!positionEqual(requested.from, candidate.from) || !positionEqual(requested.to, candidate.to)) {
        return 0;
    }

    if (requested.movedPiece.type != candidate.movedPiece.type
        || requested.movedPiece.color != candidate.movedPiece.color) {
        return 0;
    }

    if (move_requests_explicit_special_semantics(requested)) {
        return moves_match_exactly(requested, candidate);
    }

    return 1;
}

SelectionResult validateSelection(const GameState *state, Position pos) {
    Piece piece;

    if (state == NULL || !isValidPosition(pos)) {
        return SELECT_OUT_OF_BOUNDS;
    }

    piece = getPiece(&state->board, pos);
    if (piece.type == EMPTY_PIECE) {
        return SELECT_EMPTY;
    }

    if (piece.color != state->currentTurn) {
        return SELECT_OPPONENT_PIECE;
    }

    return SELECT_VALID;
}

int validateMove(const GameState *state, Move move) {
    MoveList candidates;
    int index;

    if (state == NULL || !isValidPosition(move.from) || !isValidPosition(move.to)) {
        return 0;
    }

    if (generateLegalMovesForPosition(state, move.from, &candidates) != 0) {
        return 0;
    }

    for (index = 0; index < getMoveCount(&candidates); ++index) {
        Move *candidate = getMove(&candidates, index);

        if (candidate != NULL && generated_move_matches_request(move, *candidate)) {
            return 1;
        }
    }

    return 0;
}
