#include "lab/order_book_reference.hpp"
#include <algorithm>
#include <stdexcept>

namespace lab::book {
ReferenceBook::ReferenceBook(Limits limits) : limits_(limits) {
    if(!limits.min_price || limits.min_price > limits.max_price || !limits.max_orders)
        throw std::invalid_argument("invalid reference limits");
    orders_.reserve(limits.max_orders);
}
Outcome ReferenceBook::apply(const Event& e, std::span<Trade> output) {
    const bool adding = e.kind == Kind::add, market = e.kind == Kind::match;
    if(!adding && !market && e.kind != Kind::cancel && e.kind != Kind::modify) return {Status::invalid};
    if(!market && !e.id) return {Status::invalid};
    auto incoming_side = e.side;
    auto incoming_price = e.price;
    auto position = std::find_if(orders_.begin(), orders_.end(), [&](const auto& o) { return o.id == e.id; });
    if(adding || market) {
        if((e.side != Side::buy && e.side != Side::sell) || !e.quantity ||
           (adding && (e.price < limits_.min_price || e.price > limits_.max_price))) return {Status::invalid};
        if(adding && position != orders_.end()) return {Status::duplicate};
        if(adding && orders_.size() == limits_.max_orders) return {Status::full};
        if(output.size() < orders_.size()) return {Status::output_full};
    } else {
        if(position == orders_.end()) return {Status::not_found};
        if(e.kind == Kind::cancel || !e.quantity) { orders_.erase(position); return {}; }
        if(e.price < limits_.min_price || e.price > limits_.max_price) return {Status::invalid};
        if(e.price == position->price && e.quantity <= position->quantity) {
            position->quantity = e.quantity;
            return {Status::ok, e.quantity};
        }
        if(output.size() < orders_.size()) return {Status::output_full};
        incoming_side = position->side;
        orders_.erase(position); // Appending remainder later loses old priority.
    }
    if(market) incoming_price = incoming_side == Side::buy ? limits_.max_price : limits_.min_price;
    Outcome result{Status::ok, e.quantity, 0};
    while(result.remaining) {
        auto best = orders_.end();
        for(auto it = orders_.begin(); it != orders_.end(); ++it) {
            if(it->side == incoming_side) continue;
            if(incoming_side == Side::buy ? it->price > incoming_price : it->price < incoming_price) continue;
            // Strict comparison retains the earlier vector entry at equal price.
            if(best == orders_.end() || (incoming_side == Side::buy ? it->price < best->price : it->price > best->price)) best = it;
        }
        if(best == orders_.end()) break;
        const Quantity fill = std::min(best->quantity, result.remaining);
        output[result.trade_count++] = {best->id, market ? 0 : e.id, best->price, fill};
        result.remaining -= fill;
        best->quantity -= fill;
        if(!best->quantity) orders_.erase(best);
    }
    if(!market && result.remaining) orders_.push_back({e.id, incoming_side, incoming_price, result.remaining});
    return result;
}
std::vector<Order> ReferenceBook::snapshot() const {
    auto result = orders_;
    std::stable_sort(result.begin(), result.end(), [](const Order& a, const Order& b) {
        if(a.side != b.side) return a.side == Side::buy;
        return a.side == Side::buy ? a.price > b.price : a.price < b.price;
    });
    return result;
}
} // namespace lab::book
