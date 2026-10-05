#include "internal.hpp"
namespace ac {
Result<SearchRequest> SearchRequest::create(const Position &position, std::span<const uint64_t> hashes,
                                            SearchLimits limits, Clock clock, std::stop_token stop,
                                            std::pmr::memory_resource *resource) noexcept {
    if (!clock.now || !resource || limits.budgetMs <= 0 || limits.maxDepth <= 0 || hashes.size() > MaxMoves + 1)
        return Error{Status::InvalidArgument, "Invalid search inputs or limits"};
    try {
        detail::OwnedSequence<uint64_t> owned(resource);
        if (hashes.empty()) {
            const std::array<uint64_t, 1> root{position_hash(&position)};
            owned.assign(root.begin(), root.end());
        } else
            owned.assign(hashes.begin(), hashes.end());
        return SearchRequest{position, std::move(owned), limits, clock, stop};
    } catch (const std::bad_alloc &) {
        return Error{Status::OutOfMemory, "Search request allocation failed"};
    }
}
SearchContext::SearchContext(detail::OwnedObject<detail::SearchData> data) noexcept : data_(std::move(data)) {
}
SearchContext::~SearchContext() = default;
SearchContext::SearchContext(SearchContext &&) noexcept = default;
SearchContext &SearchContext::operator=(SearchContext &&) noexcept = default;
Result<SearchContext> SearchContext::create(std::pmr::memory_resource *resource) noexcept {
    if (!resource)
        return Error{Status::InvalidArgument, "A memory resource is required"};
    try {
        return SearchContext{detail::make_owned<detail::SearchData>(resource, resource)};
    } catch (const std::bad_alloc &) {
        return Error{Status::OutOfMemory, "Search workspace allocation failed"};
    }
}
std::size_t SearchContext::allocated_bytes() const noexcept {
    return data_ ? sizeof(detail::SearchData) + AC_TT_SIZE * sizeof(TTEntry) + (AI_MAX_PLY + 1) * sizeof(MoveList) : 0;
}
Result<SearchResult> SearchContext::search(const SearchRequest &request) noexcept {
    auto *ctx = data_.get();
    if (!ctx || request.hashes().empty())
        return Error{Status::Unavailable, "Search owner or request has been moved"};
    ctx->options = {request.clock(), request.limits().budgetMs, request.limits().maxDepth, request.stop(),
                    request.hashes()};
    // Discard synchronous views and clock contexts before returning.
    struct ReleaseInputs {
        detail::SearchData *context;
        ~ReleaseInputs() {
            context->options = {};
        }
    } release{ctx};
    if (request.stop().stop_requested())
        return Error{Status::Cancelled, "Search cancelled"};
    SearchResult out{};
    Position root = request.position();
    root.hash = position_hash(&root);
    const int status = ai_search_best_move(ctx, &root, request.limits().maxDepth, request.limits().budgetMs, &out.move);
    out.nodes = ctx->nodes;
    out.completedDepth = ctx->completedDepth;
    out.elapsedMs = ai_elapsed_ms(ctx);
    out.status = ctx->failure != Status::Ok ? ctx->failure : status ? Status::Unavailable : Status::Ok;
    if (out.status != Status::Ok)
        return Error{out.status, "Search did not produce a move"};
    return out;
}
} // namespace ac
