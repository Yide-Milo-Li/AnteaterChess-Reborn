#include "internal.hpp"

namespace ac {
SearchContext *search_create(void) {
    SearchContext *ctx = static_cast<SearchContext *>(calloc(1, sizeof(*ctx)));
    if (!ctx)
        return NULL;
    ctx->transpositionTable = static_cast<TTEntry *>(calloc(AC_TT_SIZE, sizeof(TTEntry)));
    ctx->moveBuffers = static_cast<MoveList *>(calloc(AI_MAX_PLY + 1, sizeof(MoveList)));
    if (!ctx->transpositionTable || !ctx->moveBuffers) {
        search_destroy(ctx);
        return NULL;
    }
    return ctx;
}
void search_destroy(SearchContext *ctx) {
    if (!ctx)
        return;
    free(ctx->transpositionTable);
    free(ctx->moveBuffers);
    free(ctx);
}
Status search(SearchContext *ctx, const Position *p, const SearchOptions *o, SearchResult *out) {
    if (!ctx || !p || !o || !out || !o->clock.now || o->budgetMs <= 0 || o->maxDepth <= 0 || o->hashCount < 0 ||
        o->hashCount > MaxMoves + 1 || (!o->hashes && o->hashCount))
        return Status::InvalidArgument;
    memset(out, 0, sizeof(*out));
    ctx->options = *o;
    if (o->cancelled && o->cancelled(o->cancelContext))
        return out->status = Status::Cancelled;
    Position root = *p;
    root.hash = position_hash(&root);
    int status = ai_search_best_move(ctx, &root, o->maxDepth, o->budgetMs, &out->move);
    out->nodes = ctx->nodes;
    out->completedDepth = ctx->completedDepth;
    out->elapsedMs = ai_elapsed_ms(ctx);
    out->status = ctx->failure != Status::Ok ? ctx->failure : status ? Status::Unavailable : Status::Ok;
    return out->status;
}

} // namespace ac
