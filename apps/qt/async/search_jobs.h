#pragma once
#include "anteater/ai.hpp"
#include "anteater/session.hpp"
#include <QObject>
#include <QThread>
#include <atomic>
#include <memory>
#include <vector>

namespace ac {
struct SearchOutcome {
    SearchResult result{};
    uint64_t revision = 0, generation = 0;
    int budgetMs = 0;
    bool hint = false, cancelled = false;
};
class SearchJobs : public QObject {
    Q_OBJECT
  public:
    explicit SearchJobs(QObject *parent = nullptr) : QObject(parent) {
    }
    ~SearchJobs() override;
    bool busy() const {
        return thread_ != nullptr;
    }
    bool start(const SessionSnapshot &snapshot, uint64_t generation, bool hint, int budgetMs, int maxDepth);
    void cancel();
    void shutdown();
    const SearchOutcome &outcome() const {
        return outcome_;
    }
  signals:
    void completed();
    void busyChanged();

  private:
    struct Job {
        Position position{};
        std::vector<uint64_t> hashes;
        SearchOptions options{};
        SearchOutcome outcome;
        std::atomic<bool> cancelled{false};
    };
    QThread *thread_ = nullptr;
    std::shared_ptr<Job> job_;
    SearchOutcome outcome_;
};
} // namespace ac
