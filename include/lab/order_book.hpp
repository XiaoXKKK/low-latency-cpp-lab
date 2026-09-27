#pragma once
#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <optional>
#include <memory_resource>
#include <span>
#include <unordered_map>
#include <vector>

namespace lab::book {
using Id = std::uint64_t;
using Price = std::uint32_t;
using Quantity = std::uint32_t;
enum class Side { buy, sell };
enum class Kind { add, cancel, modify, match };
enum class Status { ok, invalid, duplicate, not_found, full, output_full };
struct Limits { Price min_price = 1, max_price = 1000000; std::size_t max_orders = 65536; };
struct Order {
    Id id; Side side; Price price; Quantity quantity;
    bool operator==(const Order&) const = default;
};
struct Event {
    Kind kind; Id id = 0; Side side = Side::buy; Price price = 0; Quantity quantity = 0;
    bool operator==(const Event&) const = default;
};
struct Trade {
    Id maker, taker; Price price; Quantity quantity;
    bool operator==(const Trade&) const = default;
};
struct Outcome {
    Status status = Status::ok;
    Quantity remaining = 0;
    std::size_t trade_count = 0;
    bool operator==(const Outcome&) const = default;
};

// Single-threaded owner. Maps own levels, lists own orders, ID index contains
// stable map/list iterators. Copy/move are forbidden: a copied index would refer
// to another book. Snapshots are detached values, ordered bids/asks best-first.
using LevelValue = std::pair<const Price, std::list<Order>>;
template<class Allocator = std::allocator<LevelValue>>
class BasicMapBook {
    using Levels = std::map<Price, std::list<Order>, std::less<Price>, Allocator>;
    struct Handle { typename Levels::iterator level; std::list<Order>::iterator order; };
    Limits limits_;
    Levels bids_, asks_;
    std::unordered_map<Id, Handle> index_;
    Levels& levels(Side side) { return side == Side::buy ? bids_ : asks_; }
    Handle stage(Order order);
    void erase(typename std::unordered_map<Id, Handle>::iterator it);
    Outcome cross(Side side, Price limit, Quantity quantity, Id taker, std::span<Trade> output);
    Outcome finish(Handle handle, std::span<Trade> output);
public:
    explicit BasicMapBook(Limits limits = {}, const Allocator& allocator = {});
    BasicMapBook(const BasicMapBook&) = delete;
    BasicMapBook& operator=(const BasicMapBook&) = delete;
    BasicMapBook(BasicMapBook&&) = delete;
    BasicMapBook& operator=(BasicMapBook&&) = delete;
    // For operations capable of matching, output.size() >= size() is required
    // before any state change (conservative bound: one trade per resting order).
    // Add at capacity is rejected even when it could immediately execute.
    // Match the baseline translation-unit boundary in every timed variant.
    [[gnu::noinline]] Outcome apply(const Event& event, std::span<Trade> output);
    std::size_t size() const { return index_.size(); }
    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;
    std::vector<Order> snapshot() const;
    bool invariant() const;
};
using MapBook = BasicMapBook<>;
using ResourceMapBook = BasicMapBook<std::pmr::polymorphic_allocator<LevelValue>>;
} // namespace lab::book
