#include "internal.h"
AcSearchContext *ac_search_create(void) {
    AcSearchContext *ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
        return NULL;
    ctx->transpositionTable = calloc(AC_TT_SIZE, sizeof(TTEntry));
    ctx->moveBuffers = calloc(AI_MAX_PLY + 1, sizeof(AcMoveList));
    if (!ctx->transpositionTable || !ctx->moveBuffers) {
        ac_search_destroy(ctx);
        return NULL;
    }
    return ctx;
}
void ac_search_destroy(AcSearchContext *ctx) {
    if (!ctx)
        return;
    free(ctx->transpositionTable);
    free(ctx->moveBuffers);
    free(ctx);
}
AcStatus ac_search(AcSearchContext *ctx, const AcPosition *p, const AcSearchOptions *o, AcSearchResult *out) {
    if (!ctx || !p || !o || !out || !o->clock.now || o->budgetMs <= 0 || o->maxDepth <= 0 || o->hashCount < 0 ||
        o->hashCount > AC_MAX_MOVES + 1 || (!o->hashes && o->hashCount))
        return AC_INVALID_ARGUMENT;
    memset(out, 0, sizeof(*out));
    ctx->options = *o;
    if (o->cancelled && o->cancelled(o->cancelContext))
        return out->status = AC_CANCELLED;
    AcPosition root = *p;
    root.hash = ac_position_hash(&root);
    int status = ac_ai_search_best_move(ctx, &root, o->maxDepth, o->budgetMs, &out->move);
    out->nodes = ctx->nodes;
    out->completedDepth = ctx->completedDepth;
    out->elapsedMs = ac_ai_elapsed_ms(ctx);
    out->status = ctx->failure ? ctx->failure : status ? AC_UNAVAILABLE : AC_OK;
    return out->status;
}
