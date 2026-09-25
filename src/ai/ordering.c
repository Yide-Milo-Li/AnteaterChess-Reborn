#include "internal.h"
void ac_ai_ensure_search_heuristics_ready(AcSearchContext *ctx) {
    if (ctx->searchHeuristicsReady != 0) {
        return;
    }

    memset(ctx->killerMoves, 0, sizeof(ctx->killerMoves));
    memset(ctx->killerValid, 0, sizeof(ctx->killerValid));
    memset(ctx->history, 0, sizeof(ctx->history));
    ctx->searchHeuristicsReady = 1;
}

void ac_ai_age_history_scores(AcSearchContext *ctx) {
    int color;
    int from;
    int to;

    ac_ai_ensure_search_heuristics_ready(ctx);
    for (color = 0; color < 2; ++color) {
        for (from = 0; from < AC_ROWS * AC_COLS; ++from) {
            for (to = 0; to < AC_ROWS * AC_COLS; ++to) {
                ctx->history[color][from][to] /= 2;
            }
        }
    }
}

int ac_ai_move_matches_entry(const AcMove *move, const TTEntry *entry) {
    if (move == NULL || entry == NULL || entry->from >= AC_ROWS * AC_COLS || entry->to >= AC_ROWS * AC_COLS) {
        return 0;
    }

    return ac_ai_square_index(move->from) == entry->from && ac_ai_square_index(move->to) == entry->to &&
           move->specialType == (AcSpecialMove)entry->special;
}

int ac_ai_move_equal_signature(const AcMove *lhs, const AcMove *rhs) {
    if (lhs == NULL || rhs == NULL) {
        return 0;
    }

    return ac_position_equal(lhs->from, rhs->from) && ac_position_equal(lhs->to, rhs->to) &&
           lhs->specialType == rhs->specialType;
}

void ac_ai_save_killer(AcSearchContext *ctx, int ply, AcMove move) {
    (void)ctx;
    ac_ai_ensure_search_heuristics_ready(ctx);
    if (ply >= AI_MAX_PLY || !ac_ai_is_quiet_move(&move)) {
        return;
    }

    if (ctx->killerValid[ply][0] && ac_ai_move_equal_signature(&ctx->killerMoves[ply][0], &move)) {
        return;
    }

    if (ctx->killerValid[ply][0]) {
        ctx->killerMoves[ply][1] = ctx->killerMoves[ply][0];
        ctx->killerValid[ply][1] = 1;
    }
    ctx->killerMoves[ply][0] = move;
    ctx->killerValid[ply][0] = 1;
}

void ac_ai_update_history_score(AcSearchContext *ctx, AcColor color, AcMove move, int delta) {
    int from;
    int to;
    int *cell;

    (void)ctx;
    ac_ai_ensure_search_heuristics_ready(ctx);
    if (!ac_ai_is_quiet_move(&move)) {
        return;
    }

    from = ac_ai_square_index(move.from);
    to = ac_ai_square_index(move.to);
    cell = &ctx->history[color][from][to];
    *cell += delta;
    if (*cell > AI_HISTORY_MAX) {
        *cell = AI_HISTORY_MAX;
    } else if (*cell < -AI_HISTORY_MAX) {
        *cell = -AI_HISTORY_MAX;
    }
}

int ac_ai_tactical_move_score(const AcMove *move) {
    int score;
    int captureIndex;

    score = 0;
    for (captureIndex = 0; captureIndex < move->captureCount; ++captureIndex) {
        score += ac_ai_piece_value(move->captures[captureIndex].piece.type) * 16;
    }

    score -= ac_ai_piece_value(move->movedPiece.type);
    if (move->specialType == AC_ANTEATER_CAPTURE) {
        score += 300 + move->captureCount * 120;
    }
    if (ac_ai_is_promotion_move(move)) {
        score += 850;
    }

    return score;
}

int ac_ai_quick_exchange_margin(const AcMove *move) {
    int gain;
    int risk;

    if (move == NULL) {
        return 0;
    }

    gain = ac_ai_see_initial_gain(move);
    risk = ac_ai_piece_value(move->movedPiece.type);
    if (move->specialType == AC_ANTEATER_CAPTURE && move->captureCount >= 2) {
        risk /= 2;
    }

    return gain - risk;
}

int ac_ai_move_order_score(AcSearchContext *ctx, const AcPosition *state, const AcMove *move, int ply,
                           const TTEntry *ttMove) {
    int from;
    int to;
    int score;

    (void)ctx;
    ac_ai_ensure_search_heuristics_ready(ctx);
    // TT best move first
    if (ac_ai_move_matches_entry(move, ttMove)) {
        return INT_MAX;
    }
    // capture / promotion: SEE based, good capture > killer > bad capture
    if (ac_ai_is_noisy_move(move)) {
        int quickMargin = ac_ai_quick_exchange_margin(move);
        int seeScore = quickMargin;
        int tacticalScore = ac_ai_tactical_move_score(move);

        if (!ac_ai_is_promotion_move(move) && quickMargin > -250 && quickMargin < 250) {
            seeScore = ac_ai_see_move_score(state, move);
        }
        if (seeScore >= 0 || ac_ai_is_promotion_move(move)) {
            return 1000000 + seeScore * 64 + tacticalScore;
        }
        return 220000 + seeScore * 64 + tacticalScore;
    }
    // killer move: quiet move that caused cutoff before
    if (ply < AI_MAX_PLY && ctx->killerValid[ply][0] && ac_ai_move_equal_signature(move, &ctx->killerMoves[ply][0])) {
        return 900000;
    }
    if (ply < AI_MAX_PLY && ctx->killerValid[ply][1] && ac_ai_move_equal_signature(move, &ctx->killerMoves[ply][1])) {
        return 899000;
    }

    // history score, plus small bonus for castling
    from = ac_ai_square_index(move->from);
    to = ac_ai_square_index(move->to);
    score = ctx->history[state->currentTurn][from][to];
    if (move->specialType == AC_CASTLING_KINGSIDE || move->specialType == AC_CASTLING_QUEENSIDE) {
        score += 150;
    }

    return score;
}

void ac_ai_sort_moves(AcSearchContext *ctx, const AcPosition *state, AcMoveList *list, int ply, const TTEntry *ttMove) {
    int scores[AC_MAX_MOVES];
    int index;

    for (index = 0; index < list->count; ++index) {
        scores[index] = ac_ai_move_order_score(ctx, state, &list->moves[index], ply, ttMove);
    }

    for (index = 1; index < list->count; ++index) {
        AcMove keyMove = list->moves[index];
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

void ac_ai_sort_root_moves_by_scores(AcMoveList *list, int scores[AC_MAX_MOVES]) {
    int index;

    for (index = 1; index < list->count; ++index) {
        AcMove keyMove = list->moves[index];
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
