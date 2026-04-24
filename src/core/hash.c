#include "core/hash.h"

#include <stdint.h>

#include "core/board.h"
#include "core/movelist.h"

#define FNV_OFFSET UINT64_C(1469598103934665603)
#define FNV_PRIME UINT64_C(1099511628211)

static uint64_t mix_value(uint64_t hash, uint64_t value) {
    int shift;

    for (shift = 0; shift < 64; shift += 8) {
        hash ^= (value >> shift) & UINT64_C(0xff);
        hash *= FNV_PRIME;
    }

    return hash;
}

static int absolute_value(int value) {
    if (value < 0) {
        return -value;
    }

    return value;
}

static int history_touches_square(const GameState *state, Position square) {
    int moveIndex;

    if (state == NULL) {
        return 0;
    }

    for (moveIndex = 0; moveIndex < state->moveHistory.count; ++moveIndex) {
        const Move *move = &state->moveHistory.moves[moveIndex];
        int captureIndex;

        if (positionEqual(move->from, square) || positionEqual(move->to, square)) {
            return 1;
        }

        for (captureIndex = 0; captureIndex < move->captureCount; ++captureIndex) {
            if (positionEqual(move->captures[captureIndex].pos, square)) {
                return 1;
            }
        }
    }

    return 0;
}

static int castling_right_available(const GameState *state, Color color, int kingside) {
    int row;
    Position kingStart;
    Position rookStart;
    Piece king;
    Piece rook;

    if (state == NULL || (color != WHITE && color != BLACK)) {
        return 0;
    }

    row = (color == WHITE) ? 7 : 0;
    kingStart = createPosition(row, 5);
    rookStart = createPosition(row, kingside ? COLS - 1 : 0);
    king = getPiece(&state->board, kingStart);
    rook = getPiece(&state->board, rookStart);

    return king.type == KING
        && king.color == color
        && rook.type == ROOK
        && rook.color == color
        && history_touches_square(state, kingStart) == 0
        && history_touches_square(state, rookStart) == 0;
}

static int current_side_can_capture_en_passant(
    const GameState *state,
    Position capturePos,
    Position *targetOut
) {
    int colOffset;
    int direction;
    Position target;

    if (state == NULL || (state->currentTurn != WHITE && state->currentTurn != BLACK)) {
        return 0;
    }

    direction = (state->currentTurn == WHITE) ? -1 : 1;
    target = createPosition(capturePos.row + direction, capturePos.col);
    if (!isValidPosition(target) || getPiece(&state->board, target).type != EMPTY_PIECE) {
        return 0;
    }

    for (colOffset = -1; colOffset <= 1; colOffset += 2) {
        Position from = createPosition(capturePos.row, capturePos.col + colOffset);
        Piece piece;

        if (!isValidPosition(from)) {
            continue;
        }

        piece = getPiece(&state->board, from);
        if (piece.type == ANT && piece.color == state->currentTurn) {
            if (targetOut != NULL) {
                *targetOut = target;
            }
            return 1;
        }
    }

    return 0;
}

static int en_passant_target(const GameState *state, Position *targetOut) {
    const Move *lastMove;
    Position capturePos;
    Piece capturedPiece;

    if (state == NULL || state->moveHistory.count <= 0) {
        return 0;
    }

    lastMove = &state->moveHistory.moves[state->moveHistory.count - 1];
    if (lastMove->movedPiece.type != ANT
        || lastMove->movedPiece.color == state->currentTurn
        || lastMove->from.col != lastMove->to.col
        || absolute_value(lastMove->to.row - lastMove->from.row) != 2) {
        return 0;
    }

    capturePos = lastMove->to;
    capturedPiece = getPiece(&state->board, capturePos);
    if (capturedPiece.type != ANT || capturedPiece.color != lastMove->movedPiece.color) {
        return 0;
    }

    return current_side_can_capture_en_passant(state, capturePos, targetOut);
}

void initZobrist(void) {
}

uint64_t computeHash(const GameState *state) {
    uint64_t hash;
    Position epTarget;
    int row;
    int col;

    if (state == NULL) {
        return 0;
    }

    hash = FNV_OFFSET;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece piece = getPiece(&state->board, createPosition(row, col));
            uint64_t token = (uint64_t)(piece.type + 1)
                | ((uint64_t)(piece.color + 1) << 8)
                | ((uint64_t)row << 16)
                | ((uint64_t)col << 24);

            hash = mix_value(hash, token);
        }
    }

    hash = mix_value(hash, (uint64_t)(state->currentTurn + 1));
    hash = mix_value(hash, (uint64_t)castling_right_available(state, WHITE, 1));
    hash = mix_value(hash, (uint64_t)castling_right_available(state, WHITE, 0));
    hash = mix_value(hash, (uint64_t)castling_right_available(state, BLACK, 1));
    hash = mix_value(hash, (uint64_t)castling_right_available(state, BLACK, 0));

    if (en_passant_target(state, &epTarget)) {
        hash = mix_value(hash, UINT64_C(1));
        hash = mix_value(hash, (uint64_t)epTarget.row);
        hash = mix_value(hash, (uint64_t)epTarget.col);
    } else {
        hash = mix_value(hash, UINT64_C(0));
    }

    return hash;
}

uint64_t updateHashMove(uint64_t hash, Move move) {
    hash = mix_value(hash, (uint64_t)move.from.row);
    hash = mix_value(hash, (uint64_t)move.from.col);
    hash = mix_value(hash, (uint64_t)move.to.row);
    hash = mix_value(hash, (uint64_t)move.to.col);
    hash = mix_value(hash, (uint64_t)move.specialType);
    return hash;
}
