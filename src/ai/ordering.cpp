#include "internal.hpp"

namespace ac {
void ai_ensure_search_heuristics_ready(SearchContext *ctx) {
    if (ctx->searchHeuristicsReady != 0) {
        return;
    }

    memset(ctx->killerMoves, 0, sizeof(ctx->killerMoves));
    memset(ctx->killerValid, 0, sizeof(ctx->killerValid));
    memset(ctx->history, 0, sizeof(ctx->history));
    ctx->searchHeuristicsReady = 1;
}

void ai_age_history_scores(SearchContext *ctx) {
    int color;
    int from;
    int to;

    ai_ensure_search_heuristics_ready(ctx);
    for (color = 0; color < 2; ++color) {
        for (from = 0; from < Rows * Columns; ++from) {
            for (to = 0; to < Rows * Columns; ++to) {
                ctx->history[enum_index(color)][from][to] /= 2;
            }
        }
    }
}

int ai_move_matches_entry(const Move *move, const TTEntry *entry) {
    if (move == NULL || entry == NULL || entry->from >= Rows * Columns || entry->to >= Rows * Columns) {
        return 0;
    }

    return ai_square_index(move->from) == entry->from && ai_square_index(move->to) == entry->to &&
           move->specialType == (SpecialMove)entry->special;
}

int ai_move_equal_signature(const Move *lhs, const Move *rhs) {
    if (lhs == NULL || rhs == NULL) {
        return 0;
    }

    return position_equal(lhs->from, rhs->from) && position_equal(lhs->to, rhs->to) &&
           lhs->specialType == rhs->specialType;
}

void ai_save_killer(SearchContext *ctx, int ply, Move move) {
    (void)ctx;
    ai_ensure_search_heuristics_ready(ctx);
    if (ply >= AI_MAX_PLY || !ai_is_quiet_move(&move)) {
        return;
    }

    if (ctx->killerValid[ply][0] && ai_move_equal_signature(&ctx->killerMoves[ply][0], &move)) {
        return;
    }

    if (ctx->killerValid[ply][0]) {
        ctx->killerMoves[ply][1] = ctx->killerMoves[ply][0];
        ctx->killerValid[ply][1] = 1;
    }
    ctx->killerMoves[ply][0] = move;
    ctx->killerValid[ply][0] = 1;
}

void ai_update_history_score(SearchContext *ctx, Color color, Move move, int delta) {
    int from;
    int to;
    int *cell;

    (void)ctx;
    ai_ensure_search_heuristics_ready(ctx);
    if (!ai_is_quiet_move(&move)) {
        return;
    }

    from = ai_square_index(move.from);
    to = ai_square_index(move.to);
    cell = &ctx->history[enum_index(color)][from][to];
    *cell += delta;
    if (*cell > AI_HISTORY_MAX) {
        *cell = AI_HISTORY_MAX;
    } else if (*cell < -AI_HISTORY_MAX) {
        *cell = -AI_HISTORY_MAX;
    }
}

int ai_tactical_move_score(const Move *move) {
    int score;
    int captureIndex;

    score = 0;
    for (captureIndex = 0; captureIndex < move->captureCount; ++captureIndex) {
        score += ai_piece_value(move->captures[captureIndex].piece.type) * 16;
    }

    score -= ai_piece_value(move->movedPiece.type);
    if (move->specialType == SpecialMove::AnteaterCapture) {
        score += 300 + move->captureCount * 120;
    }
    if (ai_is_promotion_move(move)) {
        score += 850;
    }

    return score;
}

int ai_quick_exchange_margin(const Move *move) {
    int gain;
    int risk;

    if (move == NULL) {
        return 0;
    }

    gain = ai_see_initial_gain(move);
    risk = ai_piece_value(move->movedPiece.type);
    if (move->specialType == SpecialMove::AnteaterCapture && move->captureCount >= 2) {
        risk /= 2;
    }

    return gain - risk;
}

int ai_move_order_score(SearchContext *ctx, const Position *state, const Move *move, int ply, const TTEntry *ttMove) {
    int from;
    int to;
    int score;

    (void)ctx;
    ai_ensure_search_heuristics_ready(ctx);
    // TT best move first
    if (ai_move_matches_entry(move, ttMove)) {
        return INT_MAX;
    }
    // capture / promotion: SEE based, good capture > killer > bad capture
    if (ai_is_noisy_move(move)) {
        int quickMargin = ai_quick_exchange_margin(move);
        int seeScore = quickMargin;
        int tacticalScore = ai_tactical_move_score(move);

        if (!ai_is_promotion_move(move) && quickMargin > -250 && quickMargin < 250) {
            seeScore = ai_see_move_score(state, move);
        }
        if (seeScore >= 0 || ai_is_promotion_move(move)) {
            return 1000000 + seeScore * 64 + tacticalScore;
        }
        return 220000 + seeScore * 64 + tacticalScore;
    }
    // killer move: quiet move that caused cutoff before
    if (ply < AI_MAX_PLY && ctx->killerValid[ply][0] && ai_move_equal_signature(move, &ctx->killerMoves[ply][0])) {
        return 900000;
    }
    if (ply < AI_MAX_PLY && ctx->killerValid[ply][1] && ai_move_equal_signature(move, &ctx->killerMoves[ply][1])) {
        return 899000;
    }

    // history score, plus small bonus for castling
    from = ai_square_index(move->from);
    to = ai_square_index(move->to);
    score = ctx->history[enum_index(state->currentTurn)][from][to];
    if (move->specialType == SpecialMove::CastlingKingside || move->specialType == SpecialMove::CastlingQueenside) {
        score += 150;
    }

    return score;
}

void ai_sort_moves(SearchContext *ctx, const Position *state, MoveList *list, int ply, const TTEntry *ttMove) {
    int scores[MaxMoves];
    int index;

    for (index = 0; index < list->count; ++index) {
        scores[index] = ai_move_order_score(ctx, state, &list->moves[index], ply, ttMove);
    }

    for (index = 1; index < list->count; ++index) {
        Move keyMove = list->moves[index];
        int keyScore = scores[index];
        int scan = index - 1;

        while (scan >= 0 && scores[scan] < keyScore) {
            list->moves[scan + 1] = list->moves[scan];
            scores[scan + 1] = scores[scan];
            --scan;
        }

        list->moves[scan + 1] = keyMove;
        scores[scan + 1] = keyScore;
    }
}

void ai_sort_root_moves_by_scores(MoveList *list, int scores[MaxMoves]) {
    int index;

    for (index = 1; index < list->count; ++index) {
        Move keyMove = list->moves[index];
        int keyScore = scores[index];
        int scan = index - 1;

        while (scan >= 0 && scores[scan] < keyScore) {
            list->moves[scan + 1] = list->moves[scan];
            scores[scan + 1] = scores[scan];
            --scan;
        }

        list->moves[scan + 1] = keyMove;
        scores[scan + 1] = keyScore;
    }
}

} // namespace ac
