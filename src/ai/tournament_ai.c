#include "tournament_ai.h"

#include "ai_search_internal.h"

#include "core/board.h"
#include "core/position.h"
#include "gameplay/endgame.h"

#include <stddef.h>

#define TOURNAMENT_MIN_MOVE_BUDGET_MS 300

static int clamp_int_local(int value, int minValue, int maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

static int abs_int(int value) {
    return value < 0 ? -value : value;
}

static int local_piece_value(PieceType type) {
    switch (type) {
        case ANT:
            return 100;
        case KNIGHT:
            return 320;
        case BISHOP:
            return 335;
        case ROOK:
            return 510;
        case QUEEN:
            return 950;
        case ANTEATER:
            return 235;
        case KING:
            return 20000;
        case EMPTY_PIECE:
        default:
            return 0;
    }
}

static int local_phase_value(PieceType type) {
    switch (type) {
        case KNIGHT:
        case BISHOP:
        case ANTEATER:
            return 1;
        case ROOK:
            return 2;
        case QUEEN:
            return 4;
        case ANT:
        case KING:
        case EMPTY_PIECE:
        default:
            return 0;
    }
}

static int is_local_promotion(const Move *move) {
    return move != NULL && isPromotionSpecialMove(move->specialType);
}

static int is_local_noisy(const Move *move) {
    return move != NULL && (move->captureCount > 0 || is_local_promotion(move));
}

static int initial_gain(const Move *move) {
    int gain;
    int index;

    if (move == NULL) {
        return 0;
    }

    gain = 0;
    for (index = 0; index < move->captureCount; ++index) {
        gain += local_piece_value(move->captures[index].piece.type);
    }
    if (is_local_promotion(move)) {
        gain += local_piece_value(QUEEN) - local_piece_value(ANT);
    }
    return gain;
}

static Position find_king_position(const Board *board, Color color) {
    int row;
    int col;

    if (board == NULL) {
        return createPosition(-1, -1);
    }

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position pos = createPosition(row, col);
            Piece piece = getPiece(board, pos);

            if (piece.type == KING && piece.color == color) {
                return pos;
            }
        }
    }
    return createPosition(-1, -1);
}

static int path_clear(const Board *board, Position from, Position target) {
    int rowStep;
    int colStep;
    Position current;

    rowStep = (target.row > from.row) ? 1 : ((target.row < from.row) ? -1 : 0);
    colStep = (target.col > from.col) ? 1 : ((target.col < from.col) ? -1 : 0);
    current = createPosition(from.row + rowStep, from.col + colStep);

    while (!positionEqual(current, target)) {
        if (!isValidPosition(current) || getPiece(board, current).type != EMPTY_PIECE) {
            return 0;
        }
        current.row += rowStep;
        current.col += colStep;
    }
    return 1;
}

static int attacks_square(const Board *board, Position from, Piece piece, Position target) {
    int rowDistance;
    int colDistance;
    int direction;

    rowDistance = abs_int(target.row - from.row);
    colDistance = abs_int(target.col - from.col);
    switch (piece.type) {
        case ANT:
            direction = (piece.color == WHITE) ? -1 : 1;
            return (target.row - from.row) == direction && colDistance == 1;
        case KNIGHT:
            return (rowDistance == 2 && colDistance == 1) || (rowDistance == 1 && colDistance == 2);
        case BISHOP:
            return rowDistance == colDistance && path_clear(board, from, target);
        case ROOK:
            return (from.row == target.row || from.col == target.col) && path_clear(board, from, target);
        case QUEEN:
            return ((from.row == target.row || from.col == target.col)
                    || rowDistance == colDistance)
                && path_clear(board, from, target);
        case KING:
            return rowDistance <= 1 && colDistance <= 1 && !positionEqual(from, target);
        case ANTEATER:
        case EMPTY_PIECE:
        default:
            return 0;
    }
}

static int square_attacked_by(const Board *board, Position target, Color attackingColor) {
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position from = createPosition(row, col);
            Piece piece = getPiece(board, from);

            if (piece.type == EMPTY_PIECE || piece.color != attackingColor) {
                continue;
            }
            if (attacks_square(board, from, piece, target)) {
                return 1;
            }
        }
    }
    return 0;
}

static int king_ring_pressure(const Board *board, Color color) {
    Position kingPos;
    Color enemy;
    int pressure;
    int rowOffset;
    int colOffset;

    kingPos = find_king_position(board, color);
    if (!isValidPosition(kingPos)) {
        return 0;
    }

    enemy = (color == WHITE) ? BLACK : WHITE;
    pressure = 0;
    for (rowOffset = -1; rowOffset <= 1; ++rowOffset) {
        for (colOffset = -1; colOffset <= 1; ++colOffset) {
            Position target = createPosition(kingPos.row + rowOffset, kingPos.col + colOffset);

            if (!isValidPosition(target) || positionEqual(target, kingPos)) {
                continue;
            }
            if (square_attacked_by(board, target, enemy)) {
                ++pressure;
            }
        }
    }
    return pressure;
}

static int board_phase(const Board *board) {
    int phase;
    int row;
    int col;

    phase = 0;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            phase += local_phase_value(getPiece(board, createPosition(row, col)).type);
        }
    }
    return clamp_int_local(phase, 0, 28);
}

static int king_safety_adjustment_for(const GameState *state, Color color, int phase) {
    Position kingPos;
    int homeRow;
    int homeDistance;
    int shieldRow;
    int score;
    int colOffset;

    kingPos = find_king_position(&state->board, color);
    if (!isValidPosition(kingPos)) {
        return 0;
    }

    homeRow = (color == WHITE) ? 7 : 0;
    homeDistance = (color == WHITE) ? (homeRow - kingPos.row) : (kingPos.row - homeRow);
    score = 0;
    if (phase >= 12 && homeDistance > 1) {
        score -= 18 * homeDistance;
    }

    shieldRow = kingPos.row + ((color == WHITE) ? -1 : 1);
    for (colOffset = -1; colOffset <= 1; ++colOffset) {
        Position shield = createPosition(shieldRow, kingPos.col + colOffset);
        Piece occupant;

        if (!isValidPosition(shield)) {
            continue;
        }
        occupant = getPiece(&state->board, shield);
        if (occupant.type != ANT || occupant.color != color) {
            score -= (phase >= 12) ? 6 : 3;
        }
    }

    score -= king_ring_pressure(&state->board, color) * ((phase >= 12) ? 10 : 6);
    return score;
}

static int tournament_evaluate_adjustment(const GameState *state) {
    int phase;

    if (state == NULL) {
        return 0;
    }

    phase = board_phase(&state->board);
    return king_safety_adjustment_for(state, WHITE, phase)
        - king_safety_adjustment_for(state, BLACK, phase);
}

static int tournament_soft_limit_ms(const GameState *state,
                                    const MoveList *rootMoves,
                                    int maxTimeMs) {
    int percent;
    int noisyCount;
    int index;

    if (maxTimeMs <= 0 || state == NULL || rootMoves == NULL) {
        return 0;
    }

    percent = 55;
    noisyCount = 0;
    for (index = 0; index < rootMoves->count; ++index) {
        if (is_local_noisy(&rootMoves->moves[index])) {
            ++noisyCount;
        }
    }
    if (rootMoves->count >= 32) {
        percent += 10;
    } else if (rootMoves->count <= 8) {
        percent -= 10;
    }
    if (rootMoves->count > 0 && noisyCount * 3 >= rootMoves->count) {
        percent += 10;
    }
    if (isInCheck(state, state->currentTurn)) {
        percent += 15;
    }
    if (king_ring_pressure(&state->board, state->currentTurn) >= 3) {
        percent += 15;
    }
    percent = clamp_int_local(percent, 35, 90);
    return clamp_int_local((maxTimeMs * percent) / 100,
        TOURNAMENT_MIN_MOVE_BUDGET_MS / 2,
        maxTimeMs);
}

static int tournament_allow_null_move(const GameState *state, int depth) {
    (void)depth;
    if (state == NULL) {
        return 1;
    }
    return king_ring_pressure(&state->board, state->currentTurn) < 3;
}

static int tournament_extend_move(const GameState *stateAfterMove,
                                  const Move *move,
                                  int depth,
                                  int givesCheck) {
    (void)stateAfterMove;
    if (move == NULL) {
        return 0;
    }
    if (givesCheck && depth <= 6) {
        return 1;
    }
    if (depth > 8) {
        return 0;
    }
    if (is_local_promotion(move)
        || (move->specialType == ANTEATER_CAPTURE && move->captureCount >= 2)) {
        return 1;
    }
    return is_local_noisy(move) && initial_gain(move) - local_piece_value(move->movedPiece.type) >= 100;
}

static int tournament_should_stop_after_depth(int elapsedMs,
                                              int timeLimitMs,
                                              int softTimeLimitMs,
                                              int rootScoreGap,
                                              int stableDepths) {
    if (elapsedMs < softTimeLimitMs) {
        return 0;
    }
    if (elapsedMs >= (timeLimitMs * 9) / 10) {
        return 1;
    }
    if (stableDepths >= 3 && rootScoreGap >= 180) {
        return 1;
    }
    if (rootScoreGap <= 45) {
        return 0;
    }
    return elapsedMs >= (timeLimitMs * 4) / 5;
}

int generateTournamentAIMoveWithBudget(const GameState *state,
                                       Move *move,
                                       int budgetMs) {
    AISearchProfile profile;

    if (state == NULL || move == NULL || budgetMs <= 0) {
        return 1;
    }

    profile = aiSearchProfileForDifficulty(DIFFICULTY_HARD);
    profile.maxDepth = 30;
    profile.softNumerator = 3;
    profile.softDenominator = 5;
    profile.lmrQuietStart = 6;
    profile.lmrDepthStart = 5;
    profile.lmrSecondStart = 10;
    profile.lmrThirdStart = 16;
    profile.nullDepthStart = 4;
    profile.nullReductionBase = 1;
    profile.nullReductionDeep = 2;
    profile.tacticalExtensionMaxDepth = 8;
    profile.evaluateAdjustment = tournament_evaluate_adjustment;
    profile.softLimitMs = tournament_soft_limit_ms;
    profile.allowNullMove = tournament_allow_null_move;
    profile.extendMove = tournament_extend_move;
    profile.shouldStopAfterDepth = tournament_should_stop_after_depth;
    return aiSearchBestMoveWithProfile(state, &profile, budgetMs, move);
}
