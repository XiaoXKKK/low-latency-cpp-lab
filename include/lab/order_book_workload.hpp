#pragma once
#include "lab/order_book.hpp"

namespace lab::book {
struct Trace {
    Limits limits;
    std::vector<Event> initial, events;
    std::vector<Outcome> outcomes;
    std::vector<Trade> trades;
    std::vector<Order> final_state;
    std::uint64_t fingerprint = 0;
    std::size_t peak_orders = 0, minimum_orders = 0;
};
// A versioned synthetic 60/25/10/5 stream. Full blocks of 100 have exact ratios;
// a final partial block is a deterministic prefix, reported by actual counts.
Trace make_trace(std::size_t initial_orders, std::size_t events, std::uint64_t seed);
} // namespace lab::book
