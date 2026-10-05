#pragma once
#include "anteater/rules.hpp"
#include "anteater/memory.hpp"
#include <span>
#include <stop_token>
namespace ac {
struct SearchLimits {
    int budgetMs = 350, maxDepth = 2;
};
class SearchRequest {
  public:
    // Input spans are copied synchronously. The nonthrowing clock context and
    // selected resource outlive this owner and synchronous searches using it.
    static Result<SearchRequest>
    create(const Position &position, std::span<const uint64_t> hashes, SearchLimits limits, Clock clock,
           std::stop_token stop = {}, std::pmr::memory_resource *resource = std::pmr::get_default_resource()) noexcept;
    SearchRequest(const SearchRequest &) = delete;
    SearchRequest &operator=(const SearchRequest &) = delete;
    SearchRequest(SearchRequest &&) noexcept = default;
    SearchRequest &operator=(SearchRequest &&) noexcept = default;
    const Position &position() const noexcept {
        return position_;
    }
    std::span<const uint64_t> hashes() const noexcept {
        return {hashes_.data(), hashes_.size()};
    }
    SearchLimits limits() const noexcept {
        return limits_;
    }
    Clock clock() const noexcept {
        return clock_;
    }
    std::stop_token stop() const noexcept {
        return stop_;
    }

  private:
    SearchRequest(const Position &position, detail::OwnedSequence<uint64_t> hashes, SearchLimits limits, Clock clock,
                  std::stop_token stop) noexcept
        : position_(position), hashes_(std::move(hashes)), limits_(limits), clock_(clock), stop_(stop) {
    }
    Position position_;
    detail::OwnedSequence<uint64_t> hashes_;
    SearchLimits limits_;
    Clock clock_;
    std::stop_token stop_;
};
struct SearchResult {
    Move move{};
    int nodes = 0, completedDepth = 0, elapsedMs = 0;
    Status status = Status::Ok;
};
namespace detail {
struct SearchData;
}
class SearchContext {
  public:
    // One invocation at a time. Moves transfer all workspaces without allocation.
    static Result<SearchContext>
    create(std::pmr::memory_resource *resource = std::pmr::get_default_resource()) noexcept;
    ~SearchContext();
    SearchContext(const SearchContext &) = delete;
    SearchContext &operator=(const SearchContext &) = delete;
    SearchContext(SearchContext &&) noexcept;
    SearchContext &operator=(SearchContext &&) noexcept;
    bool valid() const noexcept {
        return bool(data_);
    }
    Result<SearchResult> search(const SearchRequest &request) noexcept;
    std::size_t allocated_bytes() const noexcept;

  private:
    explicit SearchContext(detail::OwnedObject<detail::SearchData> data) noexcept;
    detail::OwnedObject<detail::SearchData> data_;
};
} // namespace ac
