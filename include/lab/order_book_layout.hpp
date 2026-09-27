#pragma once
#include "lab/order_book.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>

namespace lab::book {
// Experimental control: ONLY map price-level nodes use this resource. Orders
// and hash nodes retain their standard allocator. Both controls have identical
// slot storage and LIFO recycling; only the initial slot permutation differs.
class LevelSlots final : public std::pmr::memory_resource {
    std::size_t count_, bytes_ = 0, alignment_ = 0, stride_ = 0;
    std::byte* storage_ = nullptr;
    std::vector<std::size_t> free_;
    bool randomized_;
    std::uint64_t seed_, hash_ = 14695981039346656037ULL;
    double mean_gap_ = 0;
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if(!storage_) {
            alignment_ = std::max(alignment, std::size_t{64});
            stride_ = (bytes + alignment_ - 1) / alignment_ * alignment_;
            if(count_ > std::numeric_limits<std::size_t>::max() / stride_) throw std::bad_alloc();
            free_.resize(count_);
            std::iota(free_.begin(), free_.end(), 0);
            if(randomized_) {
                std::mt19937_64 rng(seed_);
                std::shuffle(free_.begin(), free_.end(), rng);
            }
            for(std::size_t i = 0; i < count_; ++i) {
                hash_ = (hash_ ^ free_[i]) * 1099511628211ULL;
                if(i) mean_gap_ += static_cast<double>((std::max(free_[i], free_[i-1]) - std::min(free_[i], free_[i-1])) * stride_);
            }
            if(count_ > 1) mean_gap_ /= count_ - 1;
            std::reverse(free_.begin(), free_.end());
            storage_ = static_cast<std::byte*>(::operator new(count_ * stride_, std::align_val_t{alignment_}));
            std::memset(storage_, 0, count_ * stride_); // First touch during setup.
            bytes_ = bytes;
        }
        if(bytes != bytes_ || alignment > alignment_ || free_.empty()) throw std::bad_alloc();
        const auto slot = free_.back(); free_.pop_back();
        return storage_ + slot * stride_;
    }
    void do_deallocate(void* pointer, std::size_t, std::size_t) override {
        free_.push_back(static_cast<std::size_t>(static_cast<std::byte*>(pointer) - storage_) / stride_);
    }
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }
public:
    LevelSlots(std::size_t count, bool randomized, std::uint64_t seed = 1729)
        : count_(count), randomized_(randomized), seed_(seed) {
        if(!count) throw std::invalid_argument("empty level slot resource");
    }
    LevelSlots(const LevelSlots&) = delete;
    LevelSlots& operator=(const LevelSlots&) = delete;
    ~LevelSlots() override { if(storage_) ::operator delete(storage_, std::align_val_t{alignment_}); }
    std::size_t storage_bytes() const { return count_ * stride_; }
    std::size_t free_list_bytes() const { return free_.capacity() * sizeof(std::size_t); }
    std::size_t stride() const { return stride_; }
    std::uint64_t permutation_hash() const { return hash_; }
    double mean_initial_gap() const { return mean_gap_; }
};
template<bool Randomized> class LayoutMapBook {
    static std::size_t slot_count(Limits limits) {
        if(limits.max_orders == std::numeric_limits<std::size_t>::max()) throw std::invalid_argument("slot capacity overflow");
        return limits.max_orders + 1;
    }
    LevelSlots slots_;
    ResourceMapBook book_;
public:
    explicit LayoutMapBook(Limits limits = {}) : slots_(slot_count(limits), Randomized), book_(limits, &slots_) {}
    Outcome apply(const Event& e, std::span<Trade> output) { return book_.apply(e, output); }
    std::size_t size() const { return book_.size(); }
    auto snapshot() const { return book_.snapshot(); }
    auto best_bid() const { return book_.best_bid(); }
    auto best_ask() const { return book_.best_ask(); }
    bool invariant() const { return book_.invariant(); }
    const LevelSlots& slots() const { return slots_; }
};
using SequentialMapBook = LayoutMapBook<false>;
using RandomizedMapBook = LayoutMapBook<true>;
} // namespace lab::book
