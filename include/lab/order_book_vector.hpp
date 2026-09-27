#pragma once
#include "lab/order_book.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <type_traits>
#include <variant>

namespace lab::book {
enum class Search { binary, branchless, linear };
// A directory entry owns a list by value. Vector moves transfer list nodes:
// std::allocator is always equal, so list node iterators remain valid. The ID
// index NEVER stores a directory pointer/index/iterator. Erasing an order looks
// up its price again, an explicit cost compared with MapBook's stable iterator.
struct DirectoryStatistics {
    std::size_t searches = 0, inserts = 0, erases = 0, insert_moves = 0, erase_moves = 0, max_capacity_bytes = 0;
    std::array<std::size_t, 5> best_rank_buckets{}; // rank 0, 1..3, 4..15, 16..63, >=64
};
template<bool BestAtEnd, Search Method, bool Collect = false> class VectorBook {
    struct Level {
        Price price;
        std::list<Order> orders;
        explicit Level(Price p) : price(p) {}
        Level(Level&&) noexcept = default;
        Level& operator=(Level&&) noexcept = default;
    };
    static_assert(std::is_nothrow_move_constructible_v<Level> && std::is_nothrow_move_assignable_v<Level>);
    using Levels = std::vector<Level>;
    using Handle = std::list<Order>::iterator;
    Limits limits_;
    [[no_unique_address]] std::conditional_t<Collect, DirectoryStatistics, std::monostate> stats_;
    Levels bids_, asks_;
    std::unordered_map<Id, Handle> index_;
    Levels& levels(Side side) { return side == Side::buy ? bids_ : asks_; }
    static bool before(Price a, Price b, Side side) {
        return (side == Side::buy) == BestAtEnd ? a < b : a > b;
    }
    static std::size_t locate_impl(const Levels& levels, Price price, Side side) {
        if constexpr(Method == Search::binary) {
            return std::lower_bound(levels.begin(), levels.end(), price,
                [side](const Level& level, Price p) { return before(level.price, p, side); }) - levels.begin();
        } else if constexpr(Method == Search::branchless) {
            // Fixed iteration count for a given length. The comparison selects
            // the base via cmov on the measured GCC build; the loop still branches.
            std::size_t base = 0, length = levels.size();
            while(length > 1) {
                const auto half = length / 2;
                base = before(levels[base + half - 1].price, price, side) ? base + half : base;
                length -= half;
            }
            return base + (length && before(levels[base].price, price, side));
        } else {
            // Same sorted layout; scan from the best end. Return lower_bound's
            // insertion position, including missing prices and both endpoints.
            if constexpr(BestAtEnd) {
                std::size_t position = levels.size();
                while(position && !before(levels[position - 1].price, price, side)) --position;
                return position;
            } else {
                std::size_t position = 0;
                while(position < levels.size() && before(levels[position].price, price, side)) ++position;
                return position;
            }
        }
    }
    std::size_t locate(const Levels& directory, Price price, Side side) {
        const auto pos = locate_impl(directory, price, side);
        if constexpr(Collect) {
            ++stats_.searches;
            const auto rank = BestAtEnd ? directory.size() - std::min(pos + 1, directory.size()) : pos;
            ++stats_.best_rank_buckets[rank == 0 ? 0 : rank < 4 ? 1 : rank < 16 ? 2 : rank < 64 ? 3 : 4];
        }
        return pos;
    }
    void erase_level(Levels& directory, std::size_t pos) {
        if constexpr(Collect) { ++stats_.erases; stats_.erase_moves += directory.size() - pos - 1; }
        directory.erase(directory.begin() + pos);
    }
    std::pair<std::size_t, bool> ensure(Side side, Price price) {
        auto& directory = levels(side);
        const auto pos = locate(directory, price, side);
        if(pos < directory.size() && directory[pos].price == price) return {pos, false};
        if constexpr(Collect) { ++stats_.inserts; stats_.insert_moves += directory.size() - pos; }
        directory.emplace(directory.begin() + pos, price);
        if constexpr(Collect) stats_.max_capacity_bytes = std::max(stats_.max_capacity_bytes, directory_capacity_bytes());
        return {pos, true};
    }
    Handle stage(Order order) {
        std::list<Order> pending;
        pending.push_back(order);
        const auto [pos, created] = ensure(order.side, order.price);
        auto& directory = levels(order.side);
        const auto handle = pending.begin();
        try { index_.emplace(order.id, handle); }
        catch(...) { if(created) erase_level(directory, pos); throw; }
        directory[pos].orders.splice(directory[pos].orders.end(), pending);
        return handle;
    }
    void unlink(Handle order) {
        const auto side = order->side;
        auto& directory = levels(side);
        const auto pos = locate(directory, order->price, side);
        directory[pos].orders.erase(order);
        if(directory[pos].orders.empty()) erase_level(directory, pos);
    }
    void erase(typename std::unordered_map<Id, Handle>::iterator it) {
        const auto order = it->second;
        index_.erase(it);
        unlink(order);
    }
    Outcome cross(Side side, Price limit, Quantity quantity, Id taker, std::span<Trade> output) {
        Outcome result{Status::ok, quantity, 0};
        auto& opposite = side == Side::buy ? asks_ : bids_;
        while(result.remaining && !opposite.empty()) {
            auto& level = BestAtEnd ? opposite.back() : opposite.front();
            if(side == Side::buy ? level.price > limit : level.price < limit) break;
            auto& maker = level.orders.front();
            const auto fill = std::min(result.remaining, maker.quantity);
            output[result.trade_count++] = {maker.id, taker, maker.price, fill};
            result.remaining -= fill; maker.quantity -= fill;
            if(!maker.quantity) erase(index_.find(maker.id));
        }
        return result;
    }
    Outcome finish(Handle order, std::span<Trade> output) {
        const Order incoming = *order;
        auto result = cross(incoming.side, incoming.price, incoming.quantity, incoming.id, output);
        if(result.remaining) order->quantity = result.remaining;
        else erase(index_.find(incoming.id));
        return result;
    }
    std::optional<Price> best(const Levels& directory) const {
        if(directory.empty()) return std::nullopt;
        return (BestAtEnd ? directory.back() : directory.front()).price;
    }
public:
    explicit VectorBook(Limits limits = {}) : limits_(limits) {
        if(!limits.min_price || limits.min_price > limits.max_price || !limits.max_orders)
            throw std::invalid_argument("invalid order book limits");
    }
    VectorBook(const VectorBook&) = delete;
    VectorBook& operator=(const VectorBook&) = delete;
    VectorBook(VectorBook&&) = delete;
    VectorBook& operator=(VectorBook&&) = delete;
    Outcome apply(const Event& e, std::span<Trade> output) {
        const auto valid_price = [&](Price p) { return p >= limits_.min_price && p <= limits_.max_price; };
        const auto valid_side = [](Side side) { return side == Side::buy || side == Side::sell; };
        if(e.kind == Kind::match) {
            if(!valid_side(e.side) || !e.quantity) return {Status::invalid};
            if(output.size() < size()) return {Status::output_full};
            return cross(e.side, e.side == Side::buy ? limits_.max_price : limits_.min_price, e.quantity, 0, output);
        }
        if(e.kind != Kind::add && e.kind != Kind::cancel && e.kind != Kind::modify) return {Status::invalid};
        if(!e.id) return {Status::invalid};
        if(e.kind == Kind::add) {
            if(!valid_side(e.side) || !valid_price(e.price) || !e.quantity) return {Status::invalid};
            if(index_.contains(e.id)) return {Status::duplicate};
            if(size() == limits_.max_orders) return {Status::full};
            if(output.size() < size()) return {Status::output_full};
            return finish(stage({e.id, e.side, e.price, e.quantity}), output);
        }
        auto it = index_.find(e.id);
        if(it == index_.end()) return {Status::not_found};
        if(e.kind == Kind::cancel || !e.quantity) { erase(it); return {}; }
        if(!valid_price(e.price)) return {Status::invalid};
        auto old = it->second;
        if(e.price == old->price && e.quantity <= old->quantity) {
            old->quantity = e.quantity;
            return {Status::ok, e.quantity};
        }
        if(output.size() < size()) return {Status::output_full};
        const auto side = old->side;
        std::list<Order> pending;
        pending.push_back({e.id, side, e.price, e.quantity});
        const auto pos = ensure(side, e.price).first;
        auto replacement = pending.begin();
        auto& orders = levels(side)[pos].orders;
        orders.splice(orders.end(), pending);
        it->second = replacement;
        unlink(old);
        return finish(replacement, output);
    }
    auto diagnostics() const requires Collect { return stats_; }
    void reset_diagnostics() requires Collect { stats_ = {}; stats_.max_capacity_bytes = directory_capacity_bytes(); }
    std::size_t size() const { return index_.size(); }
    std::optional<Price> best_bid() const { return best(bids_); }
    std::optional<Price> best_ask() const { return best(asks_); }
    std::size_t directory_capacity_bytes() const { return (bids_.capacity() + asks_.capacity()) * sizeof(Level); }
    std::vector<Order> snapshot() const {
        std::vector<Order> result;
        result.reserve(size());
        for(const auto* directory : {&bids_, &asks_}) {
            if constexpr(BestAtEnd) {
                for(auto it = directory->rbegin(); it != directory->rend(); ++it)
                    result.insert(result.end(), it->orders.begin(), it->orders.end());
            } else {
                for(const auto& level : *directory)
                    result.insert(result.end(), level.orders.begin(), level.orders.end());
            }
        }
        return result;
    }
    bool invariant() const {
        if(size() > limits_.max_orders || (best_bid() && best_ask() && *best_bid() >= *best_ask())) return false;
        std::size_t count = 0;
        for(Side side : {Side::buy, Side::sell}) {
            const auto& directory = side == Side::buy ? bids_ : asks_;
            for(std::size_t i = 0; i < directory.size(); ++i) {
                const auto& level = directory[i];
                if(level.orders.empty() || (i && !before(directory[i-1].price, level.price, side))) return false;
                for(auto order = level.orders.begin(); order != level.orders.end(); ++order) {
                    if(!order->id || !order->quantity || order->price != level.price || order->side != side ||
                       order->price < limits_.min_price || order->price > limits_.max_price) return false;
                    const auto found = index_.find(order->id);
                    if(found == index_.end() || found->second != order) return false;
                    ++count;
                }
            }
        }
        return count == size();
    }
};
using VectorFrontBook = VectorBook<false, Search::binary>;
using VectorBackBook = VectorBook<true, Search::binary>;
using BranchlessBook = VectorBook<true, Search::branchless>;
using LinearBook = VectorBook<true, Search::linear>;
} // namespace lab::book
