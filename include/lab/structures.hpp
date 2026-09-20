#pragma once
#include "lab/benchmark.hpp"
#include <array>
#include <memory>
#include <stdexcept>

namespace lab {
class SpinLock {
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
public:
    void lock() noexcept {
        while (flag_.test_and_set(std::memory_order_acquire))
            while (flag_.test(std::memory_order_relaxed)) pause_cpu();
    }
    void unlock() noexcept { flag_.clear(std::memory_order_release); }
};
// SPSC only. Capacity-1 usable slots; no allocation after construction.
// Padding is a controlled variant, not a claim that every CPU uses 64 B lines.
template<class T, std::size_t Capacity, std::size_t Padding = alignof(std::atomic<std::size_t>)>
class SpscRing {
    static_assert(Capacity >= 2 && (Capacity & (Capacity-1)) == 0);
    alignas(Padding) std::atomic<std::size_t> head_{0};
    alignas(Padding) std::atomic<std::size_t> tail_{0};
    alignas(Padding) std::array<T, Capacity> slots_{};
public:
    bool push(const T& value) noexcept {
        const auto h = head_.load(std::memory_order_relaxed);
        const auto next = (h+1) & (Capacity-1);
        if (next == tail_.load(std::memory_order_acquire)) return false;
        slots_[h] = value;
        head_.store(next, std::memory_order_release);
        return true;
    }
    bool pop(T& value) noexcept {
        const auto t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire)) return false;
        value = slots_[t];
        tail_.store((t+1) & (Capacity-1), std::memory_order_release);
        return true;
    }
};
struct Block { std::array<std::uint64_t, 8> words; };
class FixedPool {
    std::unique_ptr<Block[]> blocks_;
    std::vector<std::size_t> free_;
public:
    explicit FixedPool(std::size_t capacity): blocks_(std::make_unique<Block[]>(capacity)) {
        free_.reserve(capacity);
        for (std::size_t i=0; i<capacity; ++i) free_.push_back(i);
    }
    Block* allocate() {
        if (free_.empty()) throw std::bad_alloc();
        auto i = free_.back(); free_.pop_back(); return &blocks_[i];
    }
    // Only pointers returned by this pool, once each. Single-thread ownership.
    void deallocate(Block* p) { free_.push_back(static_cast<std::size_t>(p-blocks_.get())); }
};
} // namespace lab
