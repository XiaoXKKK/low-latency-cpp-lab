#pragma once
#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>
namespace lab {
class Arena {
    std::unique_ptr<std::byte[]> storage_;
    std::size_t capacity_, used_=0;
public:
    explicit Arena(std::size_t bytes): storage_(std::make_unique<std::byte[]>(bytes)),capacity_(bytes) {}
    void* allocate(std::size_t bytes,std::size_t alignment) {
        if(!alignment || (alignment&(alignment-1))) throw std::invalid_argument("arena alignment must be power of 2");
        void* candidate=storage_.get()+used_; auto space=capacity_-used_;
        if(!std::align(alignment,bytes,candidate,space)) throw std::bad_alloc();
        used_=static_cast<std::byte*>(candidate)-storage_.get()+bytes; return candidate;
    }
    // Caller must end all object lifetimes before reset; single-thread ownership.
    void reset() { used_=0; }
};
}
