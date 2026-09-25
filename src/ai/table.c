#include "internal.h"
TTEntry *ac_ai_tt_slot(AcSearchContext *ctx, uint64_t key) {
    return &ctx->transpositionTable[key & (AC_TT_SIZE - 1)];
}

int ac_ai_score_to_tt(int score, int ply) {
    if (score > AI_MATE - 1000) {
        return score + ply;
    }
    if (score < -AI_MATE + 1000) {
        return score - ply;
    }

    return score;
}

int ac_ai_score_from_tt(int score, int ply) {
    if (score > AI_MATE - 1000) {
        return score - ply;
    }
    if (score < -AI_MATE + 1000) {
        return score + ply;
    }

    return score;
}

int ac_ai_tt_lookup(AcSearchContext *ctx, uint64_t key, TTEntry *entry) {
    TTEntry *slot;

    slot = ac_ai_tt_slot(ctx, key);
    if (slot->key != key) {
        return 0;
    }

    if (entry != NULL) {
        *entry = *slot;
    }
    return 1;
}

void ac_ai_tt_store(AcSearchContext *ctx, uint64_t key, int depth, int ply, int score, int flag, const AcMove *bestMove,
                    unsigned char generation) {
    TTEntry *entry;
    int replace;

    entry = ac_ai_tt_slot(ctx, key);
    replace = 0;
    // empty slot or different position
    if (entry->key != key) {
        replace = 1;
        // exact result is always best
    } else if (flag == TT_FLAG_EXACT && entry->flag != TT_FLAG_EXACT) {
        replace = 1;
        // deeper search overrides shallower
    } else if (depth >= entry->depth) {
        replace = 1;
        // older generation, replace
    } else if (entry->generation != generation) {
        replace = 1;
    }

    if (!replace) {
        return;
    }

    entry->key = key;
    entry->score = ac_ai_score_to_tt(score, ply);
    entry->depth = (short)depth;
    entry->flag = (unsigned char)flag;
    entry->generation = generation;
    if (bestMove != NULL) {
        entry->from = (unsigned char)ac_ai_square_index(bestMove->from);
        entry->to = (unsigned char)ac_ai_square_index(bestMove->to);
        entry->special = (unsigned char)bestMove->specialType;
    } else {
        entry->from = 255;
        entry->to = 255;
        entry->special = (unsigned char)AC_NO_SPECIAL_MOVE;
    }
}
