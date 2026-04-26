#include "tournament_ai.h"

#include "ai_search_internal.h"

#include "core/board.h"
#include "core/position.h"
#include "gameplay/endgame.h"

#include <stddef.h>

#define TOURNAMENT_MIN_MOVE_BUDGET_MS 300
#define TOURNAMENT_URGENT_PROMOTION_DISTANCE 3
#define TOURNAMENT_PASSED_PROMOTION_DISTANCE 5

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

static int ant_promotion_distance(Position pos, Color color) {
    if (!isValidPosition(pos)) {
        return 99;
    }
    return (color == WHITE) ? pos.row : (ROWS - 1 - pos.row);
}

static Position ant_forward_position(Position pos, Color color) {
    int rowStep;

    rowStep = (color == WHITE) ? -1 : 1;
    return createPosition(pos.row + rowStep, pos.col);
}

static int ant_has_clear_promotion_lane(const Board *board, Position pos, Color color) {
    Position cursor;
    int distance;
    int step;

    if (board == NULL || !isValidPosition(pos)) {
        return 0;
    }

    distance = ant_promotion_distance(pos, color);
    if (distance <= 0 || distance > TOURNAMENT_PASSED_PROMOTION_DISTANCE) {
        return 0;
    }

    cursor = pos;
    for (step = 0; step < distance; ++step) {
        cursor = ant_forward_position(cursor, color);
        if (!isValidPosition(cursor) || getPiece(board, cursor).type != EMPTY_PIECE) {
            return 0;
        }
    }
    return 1;
}

static int is_edge_file(Position pos) {
    return isValidPosition(pos) && (pos.col <= 1 || pos.col >= COLS - 2);
}

static int move_ant_promotion_distance(const Move *move) {
    if (move == NULL || move->movedPiece.type != ANT) {
        return 99;
    }
    return ant_promotion_distance(move->to, move->movedPiece.color);
}

static int move_creates_passed_promotion_threat(const GameState *stateAfterMove,
                                                const Move *move) {
    if (stateAfterMove == NULL || move == NULL || move->movedPiece.type != ANT) {
        return 0;
    }
    return ant_promotion_distance(move->to, move->movedPiece.color)
        <= TOURNAMENT_PASSED_PROMOTION_DISTANCE
        && ant_has_clear_promotion_lane(&stateAfterMove->board,
            move->to,
            move->movedPiece.color);
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

static int local_attacker_weight(PieceType type) {
    switch (type) {
        case QUEEN:
            return 9;
        case ROOK:
            return 6;
        case BISHOP:
        case KNIGHT:
            return 4;
        case ANTEATER:
            return 3;
        case ANT:
            return 2;
        case KING:
        case EMPTY_PIECE:
        default:
            return 0;
    }
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

static int king_zone_pressure(const Board *board, Color color) {
    Position kingPos;
    Color enemy;
    int pressure;
    int row;
    int col;

    kingPos = find_king_position(board, color);
    if (!isValidPosition(kingPos)) {
        return 0;
    }

    enemy = (color == WHITE) ? BLACK : WHITE;
    pressure = 0;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position from = createPosition(row, col);
            Piece piece = getPiece(board, from);
            int distance;

            if (piece.type == EMPTY_PIECE || piece.color != enemy) {
                continue;
            }
            distance = abs_int(from.row - kingPos.row) + abs_int(from.col - kingPos.col);
            if (distance <= 5) {
                pressure += local_attacker_weight(piece.type) * (6 - distance);
            }
            if (attacks_square(board, from, piece, kingPos)) {
                pressure += local_attacker_weight(piece.type) * 8;
            }
        }
    }
    return pressure;
}

static int enemy_major_count(const Board *board, Color color) {
    Color enemy;
    int count;
    int row;
    int col;

    if (board == NULL) {
        return 0;
    }

    enemy = (color == WHITE) ? BLACK : WHITE;
    count = 0;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece piece = getPiece(board, createPosition(row, col));

            if (piece.color != enemy) {
                continue;
            }
            if (piece.type == QUEEN) {
                count += 2;
            } else if (piece.type == ROOK) {
                ++count;
            }
        }
    }
    return count;
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
    int edgeDistance;
    int enemyMajors;
    int score;
    int colOffset;

    kingPos = find_king_position(&state->board, color);
    if (!isValidPosition(kingPos)) {
        return 0;
    }

    homeRow = (color == WHITE) ? 7 : 0;
    homeDistance = (color == WHITE) ? (homeRow - kingPos.row) : (kingPos.row - homeRow);
    edgeDistance = kingPos.col;
    if ((COLS - 1 - kingPos.col) < edgeDistance) {
        edgeDistance = COLS - 1 - kingPos.col;
    }
    enemyMajors = enemy_major_count(&state->board, color);
    score = 0;
    if (phase >= 12 && homeDistance > 1) {
        score -= 18 * homeDistance;
    }
    if (homeDistance > 2) {
        score -= enemyMajors * 18 * (homeDistance - 2);
    }
    if (enemyMajors > 0 && edgeDistance <= 2) {
        score -= enemyMajors * (edgeDistance <= 1 ? 34 : 18);
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

    score -= king_ring_pressure(&state->board, color) * ((phase >= 12) ? 12 : 7);
    score -= king_zone_pressure(&state->board, color) * ((phase >= 12) ? 2 : 3);
    return score;
}

static int promotion_pressure_for(const GameState *state, Color color) {
    int score;
    int row;
    int col;

    if (state == NULL) {
        return 0;
    }

    score = 0;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position pos = createPosition(row, col);
            Piece piece = getPiece(&state->board, pos);
            int distance;
            int advanceScore;

            if (piece.type != ANT || piece.color != color) {
                continue;
            }

            distance = ant_promotion_distance(pos, color);
            if (distance <= 1) {
                advanceScore = 760;
            } else if (distance == 2) {
                advanceScore = 530;
            } else if (distance == 3) {
                advanceScore = 360;
            } else if (distance == 4) {
                advanceScore = 230;
            } else if (distance == 5) {
                advanceScore = 130;
            } else {
                advanceScore = 10 * (ROWS - 1 - distance);
            }

            if (ant_has_clear_promotion_lane(&state->board, pos, color)) {
                advanceScore += 560 - 70 * distance;
            }
            if (is_edge_file(pos)) {
                advanceScore += 35;
            }
            score += advanceScore;
        }
    }
    return score;
}

static int back_rank_invasion_pressure_for(const GameState *state, Color color) {
    int score;
    int row;
    int col;
    int homeRow;
    Position kingPos;

    if (state == NULL) {
        return 0;
    }

    homeRow = (color == WHITE) ? 7 : 0;
    kingPos = find_king_position(&state->board, color);
    score = 0;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position pos = createPosition(row, col);
            Piece piece = getPiece(&state->board, pos);
            int rowDistance;
            int campDepth;
            int kingDistance;

            if (piece.type == EMPTY_PIECE || piece.color == color) {
                continue;
            }

            rowDistance = abs_int(pos.row - homeRow);
            if (rowDistance > 2) {
                continue;
            }

            campDepth = 3 - rowDistance;
            switch (piece.type) {
                case QUEEN:
                    score -= 110 * campDepth;
                    break;
                case ROOK:
                    score -= 78 * campDepth;
                    break;
                case BISHOP:
                case KNIGHT:
                    score -= 50 * campDepth;
                    break;
                case ANTEATER:
                    score -= 32 * campDepth;
                    break;
                case ANT:
                    score -= 26 * campDepth;
                    break;
                case KING:
                case EMPTY_PIECE:
                default:
                    break;
            }
            if (isValidPosition(kingPos)) {
                kingDistance = abs_int(pos.row - kingPos.row) + abs_int(pos.col - kingPos.col);
                if (kingDistance <= 4) {
                    score -= local_attacker_weight(piece.type) * (5 - kingDistance) * 10;
                }
                if (attacks_square(&state->board, pos, piece, kingPos)) {
                    score -= local_attacker_weight(piece.type) * 32;
                }
            }
        }
    }
    return score;
}

static int king_crisis_score_for(const GameState *state, Color color) {
    int score;

    if (state == NULL) {
        return 0;
    }

    score = king_ring_pressure(&state->board, color) * 32;
    score += king_zone_pressure(&state->board, color) * 4;
    score -= back_rank_invasion_pressure_for(state, color);
    if (isInCheck(state, color)) {
        score += 220;
    }
    return score;
}

static int side_has_king_crisis(const GameState *state, Color color) {
    return king_crisis_score_for(state, color) >= 260;
}

static int side_has_urgent_promotion_threat(const GameState *state, Color color) {
    Color enemy;
    int row;
    int col;

    if (state == NULL) {
        return 0;
    }

    enemy = (color == WHITE) ? BLACK : WHITE;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position pos = createPosition(row, col);
            Piece piece = getPiece(&state->board, pos);

            if (piece.type == ANT
                && piece.color == enemy
                && (ant_promotion_distance(pos, enemy) <= TOURNAMENT_URGENT_PROMOTION_DISTANCE
                    || ant_has_clear_promotion_lane(&state->board, pos, enemy))) {
                return 1;
            }
        }
    }
    return 0;
}

static int tournament_evaluate_adjustment(const GameState *state) {
    int phase;

    if (state == NULL) {
        return 0;
    }

    phase = board_phase(&state->board);
    return king_safety_adjustment_for(state, WHITE, phase)
        - king_safety_adjustment_for(state, BLACK, phase)
        + promotion_pressure_for(state, WHITE)
        - promotion_pressure_for(state, BLACK)
        + back_rank_invasion_pressure_for(state, WHITE)
        - back_rank_invasion_pressure_for(state, BLACK);
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

    percent = 32;
    noisyCount = 0;
    for (index = 0; index < rootMoves->count; ++index) {
        if (is_local_noisy(&rootMoves->moves[index])) {
            ++noisyCount;
        }
    }
    if (rootMoves->count >= 32) {
        percent += 4;
    } else if (rootMoves->count <= 8) {
        percent -= 6;
    }
    if (rootMoves->count > 0 && noisyCount * 3 >= rootMoves->count) {
        percent += 8;
    }
    if (isInCheck(state, state->currentTurn)) {
        percent += 28;
    }
    if (king_ring_pressure(&state->board, state->currentTurn) >= 3) {
        percent += 18;
    }
    if (side_has_king_crisis(state, state->currentTurn)) {
        percent += 24;
    }
    if (side_has_urgent_promotion_threat(state, state->currentTurn)) {
        percent += 25;
    }
    percent = clamp_int_local(percent, 22, 88);
    return clamp_int_local((maxTimeMs * percent) / 100,
        TOURNAMENT_MIN_MOVE_BUDGET_MS / 2,
        maxTimeMs);
}

static int tournament_allow_null_move(const GameState *state, int depth) {
    (void)depth;
    if (state == NULL) {
        return 1;
    }
    return king_ring_pressure(&state->board, state->currentTurn) < 3
        && !side_has_king_crisis(state, state->currentTurn)
        && !side_has_urgent_promotion_threat(state, state->currentTurn);
}

static int tournament_extend_move(const GameState *stateAfterMove,
                                  const Move *move,
                                  int depth,
                                  int givesCheck) {
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
        || move_ant_promotion_distance(move) <= TOURNAMENT_URGENT_PROMOTION_DISTANCE
        || move_creates_passed_promotion_threat(stateAfterMove, move)
        || (move->specialType == ANTEATER_CAPTURE && move->captureCount >= 2)) {
        return 1;
    }
    if (stateAfterMove != NULL
        && (king_ring_pressure(&stateAfterMove->board, stateAfterMove->currentTurn) >= 4
            || side_has_king_crisis(stateAfterMove, stateAfterMove->currentTurn)
            || side_has_urgent_promotion_threat(stateAfterMove, stateAfterMove->currentTurn))) {
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
    if (stableDepths >= 2 && rootScoreGap >= 220) {
        return 1;
    }
    if (rootScoreGap <= 35 && elapsedMs < (timeLimitMs * 7) / 10) {
        return 0;
    }
    return elapsedMs >= (timeLimitMs * 7) / 10;
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
    profile.lmrQuietStart = 7;
    profile.lmrDepthStart = 6;
    profile.lmrSecondStart = 12;
    profile.lmrThirdStart = 18;
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
