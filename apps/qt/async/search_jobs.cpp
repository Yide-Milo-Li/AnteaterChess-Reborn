#include "search_jobs.h"
#include "runtime/runtime.h"
#include <algorithm>
namespace ac {
SearchJobs::~SearchJobs() {
    shutdown();
}
Status SearchJobs::start(const SessionSnapshot &s, uint64_t generation, bool hint, int budget, int depth) noexcept {
    if (busy())
        return Status::Unavailable;
    if (!resource_)
        return Status::InvalidArgument;
    try {
        std::stop_source stop;
        auto owned = SearchRequest::create(s.position, {s.hashes.data(), s.hashes.size()}, {std::max(1, budget), depth},
                                           Clock{monotonicMilliseconds, nullptr}, stop.get_token(), resource_);
        if (auto *error = std::get_if<Error>(&owned))
            return error->status;
        auto j = std::allocate_shared<Job>(std::pmr::polymorphic_allocator<Job>{resource_},
                                           std::move(std::get<SearchRequest>(owned)), std::move(stop), resource_);
        j->outcome.gameId = s.gameId;
        j->outcome.revision = s.revision;
        j->outcome.generation = generation;
        j->outcome.hint = hint;
        j->outcome.budgetMs = std::max(1, budget);
        // Capture only owned inputs and cancellation. No Session/UI access.
        std::unique_ptr<QThread> prepared(QThread::create([j] {
            auto owner = SearchContext::create(j->resource);
            if (auto *error = std::get_if<Error>(&owner))
                j->outcome.result.status = error->status;
            else {
                auto result = std::get<SearchContext>(owner).search(j->request);
                if (auto *error = std::get_if<Error>(&result))
                    j->outcome.result.status = error->status;
                else
                    j->outcome.result = std::get<SearchResult>(result);
            }
        }));
        auto *thread = prepared.get();
        thread->setParent(this);
        connect(thread, &QThread::finished, this, [this, j, thread] {
            // A completion already queued before shutdown survives disconnect.
            if (thread_ != thread || job_ != j)
                return;
            thread->wait();
            outcome_ = j->outcome;
            outcome_.cancelled = j->stop.stop_requested();
            thread_ = nullptr;
            job_.reset();
            thread->deleteLater();
            emit busyChanged();
            emit completed();
        });
        // Publish only after request, job, thread and connection are prepared.
        job_ = j;
        thread_ = prepared.release();
        thread->start();
        emit busyChanged();
        return Status::Ok;
    } catch (const std::bad_alloc &) {
        return Status::OutOfMemory;
    }
}
void SearchJobs::cancel() {
    if (job_)
        job_->stop.request_stop();
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
