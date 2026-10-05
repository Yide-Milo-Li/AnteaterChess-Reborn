#pragma once
#include <memory>
#include <memory_resource>
#include <utility>
#include <iterator>

namespace ac::detail {
// The resource must outlive this pointer and every allocation it owns. Keeping
// the resource in the deleter makes moves preserve the original deallocation path.
template <class T> struct ResourceDeleter {
    std::pmr::memory_resource *resource = std::pmr::get_default_resource();
    void operator()(T *object) const noexcept {
        if (!object)
            return;
        std::destroy_at(object);
        std::pmr::polymorphic_allocator<T>{resource}.deallocate(object, 1);
    }
};
template <class T> using OwnedObject = std::unique_ptr<T, ResourceDeleter<T>>;
template <class T, class... Args> OwnedObject<T> make_owned(std::pmr::memory_resource *resource, Args &&...args) {
    std::pmr::polymorphic_allocator<T> allocator{resource};
    T *object = allocator.allocate(1);
    try {
        std::construct_at(object, std::forward<Args>(args)...);
    } catch (...) {
        allocator.deallocate(object, 1);
        throw;
    }
    return OwnedObject<T>{object, ResourceDeleter<T>{resource}};
}

template <class T> struct ResourceArrayDeleter {
    std::pmr::memory_resource *resource;
    std::size_t count = 0;
    void operator()(T *data) const noexcept {
        if (!data)
            return;
        std::destroy_n(data, count);
        std::pmr::polymorphic_allocator<T>{resource}.deallocate(data, count);
    }
};
// Exact-sized owned data has no hidden iterator-proxy allocation. Construction
// and moves remain allocation-free in MSVC Debug as well as other configurations.
template <class T> class OwnedSequence {
  public:
    explicit OwnedSequence(std::pmr::memory_resource *resource = std::pmr::get_default_resource()) noexcept
        : data_(nullptr, ResourceArrayDeleter<T>{resource}) {
    }
    OwnedSequence(const OwnedSequence &) = delete;
    OwnedSequence &operator=(const OwnedSequence &) = delete;
    OwnedSequence(OwnedSequence &&) noexcept = default;
    OwnedSequence &operator=(OwnedSequence &&) noexcept = default;
    void resize(std::size_t count) {
        auto *resource = data_.get_deleter().resource;
        OwnedSequence prepared(resource);
        if (count) {
            std::pmr::polymorphic_allocator<T> allocator(resource);
            auto *data = allocator.allocate(count);
            try {
                std::uninitialized_value_construct_n(data, count);
            } catch (...) {
                allocator.deallocate(data, count);
                throw;
            }
            prepared.data_ = {
                data, ResourceArrayDeleter<T>{resource, count}
            };
        }
        *this = std::move(prepared);
    }
    template <class Iterator> void assign(Iterator first, Iterator last) {
        const auto count = first == last ? std::size_t{0} : static_cast<std::size_t>(std::distance(first, last));
        auto *resource = data_.get_deleter().resource;
        OwnedSequence prepared(resource);
        if (count) {
            std::pmr::polymorphic_allocator<T> allocator(resource);
            auto *data = allocator.allocate(count);
            try {
                std::uninitialized_copy(first, last, data);
            } catch (...) {
                allocator.deallocate(data, count);
                throw;
            }
            prepared.data_ = {
                data, ResourceArrayDeleter<T>{resource, count}
            };
        }
        *this = std::move(prepared);
    }
    std::size_t size() const noexcept {
        return data_ ? data_.get_deleter().count : 0;
    }
    bool empty() const noexcept {
        return size() == 0;
    }
    T *data() noexcept {
        return data_.get();
    }
    const T *data() const noexcept {
        return data_.get();
    }
    T *begin() noexcept {
        return data();
    }
    const T *begin() const noexcept {
        return data();
    }
    T *end() noexcept {
        return empty() ? data() : data() + size();
    }
    const T *end() const noexcept {
        return empty() ? data() : data() + size();
    }
    T &operator[](std::size_t index) noexcept {
        return data()[index];
    }
    const T &operator[](std::size_t index) const noexcept {
        return data()[index];
    }
    const T &back() const noexcept {
        return data()[size() - 1];
    }

  private:
    std::unique_ptr<T[], ResourceArrayDeleter<T>> data_;
};
} // namespace ac::detail
