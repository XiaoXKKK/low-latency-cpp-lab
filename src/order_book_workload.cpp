#include "lab/order_book_workload.hpp"
#include "lab/order_book_reference.hpp"
#include <algorithm>
#include <array>
#include <random>
#include <stdexcept>

namespace lab::book {
Trace make_trace(std::size_t initial_orders, std::size_t events, std::uint64_t seed) {
    if(!initial_orders || initial_orders > 10000 || !events || events > 100000)
        throw std::invalid_argument("order book: initial orders 1..10000, trace events 1..100000");
    Trace trace;
    trace.limits = {1, 20000, initial_orders + events + 1};
    trace.minimum_orders = trace.peak_orders = initial_orders;
    ReferenceBook reference(trace.limits);
    std::vector<Trade> scratch(trace.limits.max_orders);
    std::mt19937_64 rng(seed);
    const auto resting_price = [&](Side side) -> Price {
        return side == Side::buy ? 9500 + rng() % 500 : 10001 + rng() % 500;
    };
    Id next_id = 1;
    for(std::size_t i = 0; i < initial_orders; ++i) {
        const auto side = i % 2 ? Side::sell : Side::buy;
        Event e{Kind::add, next_id++, side, resting_price(side), static_cast<Quantity>(10 + rng() % 91)};
        auto outcome = reference.apply(e, scratch);
        if(outcome.status != Status::ok || outcome.trade_count) throw std::runtime_error("invalid initial trace");
        trace.initial.push_back(e);
    }
    trace.events.reserve(events); trace.outcomes.reserve(events);
    std::array<Kind, 100> kinds;
    std::fill_n(kinds.begin(), 60, Kind::add);
    std::fill_n(kinds.begin() + 60, 25, Kind::cancel);
    std::fill_n(kinds.begin() + 85, 10, Kind::modify);
    std::fill_n(kinds.begin() + 95, 5, Kind::match);
    for(std::size_t i = 0; i < events; ++i) {
        // Explicit Fisher-Yates avoids implementation-dependent std::shuffle.
        if(i % 100 == 0) for(std::size_t j = kinds.size(); j > 1; --j) std::swap(kinds[j - 1], kinds[rng() % j]);
        Event e{kinds[i % 100]};
        if(e.kind == Kind::add) {
            e.id = next_id++;
            e.side = rng() % 2 ? Side::buy : Side::sell;
            e.price = resting_price(e.side);
            if(rng() % 10 < 3) e.price = e.side == Side::buy ? 10020 + rng() % 481 : 9500 + rng() % 481;
            e.quantity = 10 + rng() % 91;
        } else if(e.kind == Kind::match) {
            e.side = rng() % 2 ? Side::buy : Side::sell;
            e.quantity = 10 + rng() % 491;
        } else if(reference.size()) {
            const Order target = reference.at(rng() % reference.size());
            e.id = target.id; e.side = target.side; e.price = target.price;
            if(e.kind == Kind::modify) {
                e.quantity = rng() % 2 ? std::max<Quantity>(1, target.quantity / 2) : target.quantity + 20;
                if(rng() % 10 < 3) e.price = resting_price(target.side);
            }
        } else { e.id = next_id + 1; e.price = 10000; e.quantity = 1; }
        const auto outcome = reference.apply(e, scratch);
        trace.events.push_back(e); trace.outcomes.push_back(outcome);
        trace.trades.insert(trace.trades.end(), scratch.begin(), scratch.begin() + outcome.trade_count);
        trace.peak_orders = std::max(trace.peak_orders, reference.size());
        trace.minimum_orders = std::min(trace.minimum_orders, reference.size());
    }
    trace.final_state = reference.snapshot();
    // FNV-1a, fixed little-endian field encoding, never object padding/std::hash.
    trace.fingerprint = 14695981039346656037ULL;
    auto hash = [&](std::uint64_t value) {
        for(unsigned i = 0; i < 8; ++i) { trace.fingerprint ^= (value >> (8 * i)) & 255; trace.fingerprint *= 1099511628211ULL; }
    };
    hash(1); // workload version
    hash(initial_orders); hash(events); hash(trace.limits.min_price); hash(trace.limits.max_price);
    for(const auto* sequence : {&trace.initial, &trace.events}) for(const auto& e : *sequence) {
        hash(static_cast<unsigned>(e.kind)); hash(e.id); hash(static_cast<unsigned>(e.side)); hash(e.price); hash(e.quantity);
    }
    return trace;
}
} // namespace lab::book
