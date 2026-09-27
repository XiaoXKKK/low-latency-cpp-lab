#include "lab/order_book.hpp"
#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace lab::book {
namespace {
bool valid(Side side) { return side == Side::buy || side == Side::sell; }
}
template<class Allocator>
BasicMapBook<Allocator>::BasicMapBook(Limits limits, const Allocator& allocator)
    : limits_(limits), bids_(allocator), asks_(allocator) {
    if(!limits.min_price || limits.min_price > limits.max_price || !limits.max_orders)
        throw std::invalid_argument("invalid order book limits");
}
template<class Allocator>
typename BasicMapBook<Allocator>::Handle BasicMapBook<Allocator>::stage(Order order) {
    // Prepare all potentially allocating nodes before matching. An allocation
    // failure rolls back the newly-created level and leaves the book unchanged.
    std::list<Order> pending;
    pending.push_back(order);
    auto& side = levels(order.side);
    auto [level, created] = side.try_emplace(order.price);
    Handle handle{level, pending.begin()};
    try { index_.emplace(order.id, handle); }
    catch(...) { if(created) side.erase(level); throw; }
    level->second.splice(level->second.end(), pending);
    return handle;
}
template<class Allocator>
void BasicMapBook<Allocator>::erase(typename std::unordered_map<Id, Handle>::iterator it) {
    auto handle = it->second;
    auto& side = levels(handle.order->side);
    index_.erase(it);
    handle.level->second.erase(handle.order);
    if(handle.level->second.empty()) side.erase(handle.level);
}
template<class Allocator>
Outcome BasicMapBook<Allocator>::cross(Side side, Price limit, Quantity quantity, Id taker, std::span<Trade> output) {
    Outcome result{Status::ok, quantity, 0};
    auto& opposite = side == Side::buy ? asks_ : bids_;
    while(result.remaining && !opposite.empty()) {
        auto level = side == Side::buy ? opposite.begin() : std::prev(opposite.end());
        if(side == Side::buy ? level->first > limit : level->first < limit) break;
        auto& maker = level->second.front();
        const auto fill = std::min(result.remaining, maker.quantity);
        output[result.trade_count++] = {maker.id, taker, maker.price, fill};
        result.remaining -= fill;
        maker.quantity -= fill;
        if(!maker.quantity) erase(index_.find(maker.id));
    }
    return result;
}
template<class Allocator>
Outcome BasicMapBook<Allocator>::finish(Handle handle, std::span<Trade> output) {
    const Order incoming = *handle.order;
    auto result = cross(incoming.side, incoming.price, incoming.quantity, incoming.id, output);
    if(result.remaining) handle.order->quantity = result.remaining;
    else erase(index_.find(incoming.id));
    return result;
}
template<class Allocator>
Outcome BasicMapBook<Allocator>::apply(const Event& e, std::span<Trade> output) {
    const auto valid_price = [&](Price p) { return p >= limits_.min_price && p <= limits_.max_price; };
    if(e.kind == Kind::match) {
        if(!valid(e.side) || !e.quantity) return {Status::invalid};
        if(output.size() < size()) return {Status::output_full};
        return cross(e.side, e.side == Side::buy ? limits_.max_price : limits_.min_price, e.quantity, 0, output);
    }
    if(e.kind != Kind::add && e.kind != Kind::cancel && e.kind != Kind::modify) return {Status::invalid};
    if(!e.id) return {Status::invalid};
    if(e.kind == Kind::add) {
        if(!valid(e.side) || !valid_price(e.price) || !e.quantity) return {Status::invalid};
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
    if(e.price == old.order->price && e.quantity <= old.order->quantity) {
        old.order->quantity = e.quantity;
        return {Status::ok, e.quantity};
    }
    if(output.size() < size()) return {Status::output_full};
    // Allocate before replacing; even a full book can Modify in place. Linking
    // the replacement first also preserves the same-price level iterator.
    const auto side = old.order->side;
    std::list<Order> pending;
    pending.push_back({e.id, side, e.price, e.quantity});
    auto level = levels(side).try_emplace(e.price).first;
    Handle replacement{level, pending.begin()};
    level->second.splice(level->second.end(), pending);
    it->second = replacement;
    old.level->second.erase(old.order);
    if(old.level->second.empty()) levels(side).erase(old.level);
    return finish(replacement, output);
}
template<class Allocator>
std::optional<Price> BasicMapBook<Allocator>::best_bid() const {
    if(bids_.empty()) return std::nullopt;
    return bids_.rbegin()->first;
}
template<class Allocator>
std::optional<Price> BasicMapBook<Allocator>::best_ask() const {
    if(asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}
template<class Allocator>
std::vector<Order> BasicMapBook<Allocator>::snapshot() const {
    std::vector<Order> result;
    result.reserve(size());
    for(auto it = bids_.rbegin(); it != bids_.rend(); ++it)
        result.insert(result.end(), it->second.begin(), it->second.end());
    for(const auto& [price, orders] : asks_) {
        (void)price;
        result.insert(result.end(), orders.begin(), orders.end());
    }
    return result;
}
template<class Allocator>
bool BasicMapBook<Allocator>::invariant() const {
    if(size() > limits_.max_orders || (best_bid() && best_ask() && *best_bid() >= *best_ask())) return false;
    std::size_t count = 0;
    for(Side side : {Side::buy, Side::sell}) {
        const auto& levels = side == Side::buy ? bids_ : asks_;
        for(auto level = levels.begin(); level != levels.end(); ++level) {
            if(level->second.empty()) return false;
            for(auto order = level->second.begin(); order != level->second.end(); ++order) {
                if(!order->id || !order->quantity || order->price != level->first || order->side != side ||
                   order->price < limits_.min_price || order->price > limits_.max_price) return false;
                auto it = index_.find(order->id);
                if(it == index_.end() || it->second.level != level || it->second.order != order) return false;
                ++count;
            }
        }
    }
    return count == size();
}
template class BasicMapBook<std::allocator<LevelValue>>;
template class BasicMapBook<std::pmr::polymorphic_allocator<LevelValue>>;
} // namespace lab::book
