#pragma once
#include "anteater/ai.hpp"
#include "anteater/session.hpp"
#include <QObject>
#include <QThread>
#include <memory>

namespace ac {
struct SearchOutcome {
    SearchResult result{};
    uint64_t gameId = 0, revision = 0, generation = 0;
    int budgetMs = 0;
    bool hint = false, cancelled = false;
};
class SearchJobs : public QObject {
    Q_OBJECT
  public:
    explicit SearchJobs(QObject *parent = nullptr,
                        std::pmr::memory_resource *resource = std::pmr::get_default_resource())
        : QObject(parent), resource_(resource) {
    }
    ~SearchJobs() override;
    bool busy() const {
        return thread_ != nullptr;
    }
    Status start(const SessionSnapshot &snapshot, uint64_t generation, bool hint, int budgetMs, int maxDepth) noexcept;
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
        Job(SearchRequest owned, std::stop_source source, std::pmr::memory_resource *resource)
            : request(std::move(owned)), stop(std::move(source)), resource(resource) {
        }
        SearchRequest request;
        std::stop_source stop;
        std::pmr::memory_resource *resource;
        SearchOutcome outcome{};
    };
    QThread *thread_ = nullptr;
    std::shared_ptr<Job> job_;
    SearchOutcome outcome_;
    std::pmr::memory_resource *resource_;
};
} // namespace ac
