#include "search_jobs.h"
#include "runtime/runtime.h"
#include <algorithm>

using namespace ac;
namespace ac {
SearchJobs::~SearchJobs() {
    shutdown();
}
bool SearchJobs::start(const Snapshot &s, uint64_t generation, bool hint, int budget, int depth) {
    if (busy())
        return false;
    auto j = std::make_shared<Job>();
    j->position = s.position;
    j->hashes.assign(s.hashes, s.hashes + s.historyCount + 1);
    j->outcome.revision = s.revision;
    j->outcome.generation = generation;
    j->outcome.hint = hint;
    j->outcome.budgetMs = std::max(1, budget);
    j->options.clock = {monotonicMilliseconds, nullptr};
    j->options.budgetMs = j->outcome.budgetMs;
    j->options.maxDepth = depth;
    j->options.hashes = j->hashes.data();
    j->options.hashCount = int(j->hashes.size());
    j->options.cancelContext = j.get();
    j->options.cancelled = [](void *p) -> int { return static_cast<Job *>(p)->cancelled.load(); };
    job_ = j;
    // The worker captures only owned search data, never the UI or live session.
    thread_ = QThread::create([j] {
        SearchContext *ctx = search_create();
        j->outcome.result.status =
            ctx ? search(ctx, &j->position, &j->options, &j->outcome.result) : Status::OutOfMemory;
        search_destroy(ctx);
    });
    thread_->setParent(this);
    QThread *thread = thread_;
    connect(thread, &QThread::finished, this, [this, j, thread] {
        // Disconnect cannot remove a completion already queued before shutdown.
        // Check both identities before touching a retired or reused thread address.
        if (thread_ != thread || job_ != j)
            return;
        thread->wait();
        outcome_ = j->outcome;
        outcome_.cancelled = j->cancelled.load();
        thread_ = nullptr;
        job_.reset();
        thread->deleteLater();
        emit busyChanged();
        emit completed();
    });
    thread->start();
    emit busyChanged();
    return true;
}
void SearchJobs::cancel() {
    if (job_)
        job_->cancelled.store(true);
}
void SearchJobs::shutdown() {
    cancel();
    if (thread_) {
        disconnect(thread_, nullptr, this, nullptr);
        thread_->wait();
        delete thread_;
        thread_ = nullptr;
        job_.reset();
    }
}
} // namespace ac
