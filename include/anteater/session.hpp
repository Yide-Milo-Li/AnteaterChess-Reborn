#pragma once
#include "anteater/rules.hpp"
#include "anteater/policy.hpp"
#include "anteater/memory.hpp"
#include <memory_resource>
#include <vector>

namespace ac {
enum class SessionPhase { Idle, Active, Finished };
// Polling clocks reads this value without allocating or borrowing Session data.
struct SessionState {
    Position position{};
    GameConfig config{};
    SessionPhase phase = SessionPhase::Idle;
    GameResult result = GameResult::None;
    uint64_t revision = 0, gameId = 0;
    int historyCount = 0;
    int64_t elapsedMs = 0;
    std::array<int, 2> remaining{}, tournamentRemainingMs{};
    bool operator==(const SessionState &) const = default;
};
// The resource outlives this owning snapshot, including after its originating
// Session is modified, moved or destroyed. Copying requires an explicit resource.
struct SessionSnapshot : SessionState {
    explicit SessionSnapshot(std::pmr::memory_resource *resource = std::pmr::get_default_resource())
        : history(resource), hashes(resource) {
    }
    SessionSnapshot(const SessionSnapshot &) = delete;
    SessionSnapshot &operator=(const SessionSnapshot &) = delete;
    SessionSnapshot(SessionSnapshot &&) noexcept = default;
    SessionSnapshot &operator=(SessionSnapshot &&) noexcept = default;
    detail::OwnedSequence<Move> history;
    detail::OwnedSequence<uint64_t> hashes;
};
struct SessionOptions {
    Clock clock{};
    std::pmr::memory_resource *resource = std::pmr::get_default_resource();
};
namespace detail {
struct SessionData;
}
class Session {
  public:
    // Clock callbacks must not throw. The clock context and resource outlive
    // this owner; the resource also outlives snapshots that use it.
    static Result<Session> create(SessionOptions options) noexcept;
    ~Session();
    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;
    Session(Session &&) noexcept;
    Session &operator=(Session &&) noexcept;
    bool valid() const noexcept {
        return bool(data_);
    }
    SessionState state() const noexcept;
    Result<SessionSnapshot> snapshot(std::pmr::memory_resource *resource = nullptr) const noexcept;
    Status start(const GameConfig &config) noexcept;
    Status tick() noexcept;
    Status submit(MoveRequest request) noexcept;
    Status submit_ai(Move move, uint64_t revision, int budgetMs, int elapsedMs) noexcept;
    Status undo() noexcept;
    Status finish() noexcept;
    Result<bool> promotion(MoveRequest request) const noexcept;
    int ai_budget() const noexcept;

  private:
    explicit Session(detail::OwnedObject<detail::SessionData> data) noexcept;
    detail::OwnedObject<detail::SessionData> data_;
};
const char *status_message(Status status);
} // namespace ac
