#pragma once
#include <memory>
#include <memory_resource>
#include <utility>

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
} // namespace ac::detail
