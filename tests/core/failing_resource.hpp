#pragma once
#include <cassert>
#include <memory_resource>
#include <new>

// Fail an individual allocation and track outstanding blocks. Tests keep this
// resource alive until every Session, snapshot, request and workspace is gone.
class FailingResource final : public std::pmr::memory_resource {
  public:
    std::size_t attempts = 0, live = 0, failAt = 0;
    void failNext() {
        failAt = attempts + 1;
    }
    ~FailingResource() override {
        assert(live == 0);
    }

  private:
    void *do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (++attempts == failAt)
            throw std::bad_alloc{};
        void *block = std::pmr::new_delete_resource()->allocate(bytes, alignment);
        ++live;
        return block;
    }
    void do_deallocate(void *block, std::size_t bytes, std::size_t alignment) override {
        assert(live > 0);
        --live;
        std::pmr::new_delete_resource()->deallocate(block, bytes, alignment);
    }
    bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override {
        return this == &other;
    }
};
