#pragma once
#include "lab/order_book.hpp"

namespace lab::book {
// Deliberately slow semantic oracle, never a performance comparison variant.
// Insertion order in this vector IS time priority; every fill scans all orders.
class ReferenceBook {
    Limits limits_;
    std::vector<Order> orders_;
public:
    explicit ReferenceBook(Limits limits = {});
    Outcome apply(const Event&, std::span<Trade>);
    std::size_t size() const { return orders_.size(); }
    Order at(std::size_t index) const { return orders_.at(index); }
    std::vector<Order> snapshot() const;
};
} // namespace lab::book
