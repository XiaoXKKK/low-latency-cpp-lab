#include "lab/order_book.hpp"
#include "lab/order_book_reference.hpp"
#include "lab/order_book_workload.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <stdexcept>
#include <type_traits>
#define CHECK(value) do { if(!(value)) throw std::runtime_error(#value); } while(false)
using namespace lab::book;

struct Harness {
    MapBook book;
    ReferenceBook oracle;
    std::vector<Trade> actual, expected;
    std::vector<Trade> last;
    explicit Harness(Limits limits = {1, 200, 128}) : book(limits), oracle(limits), actual(limits.max_orders), expected(limits.max_orders) {}
    Outcome apply(Event e, bool short_output = false) {
        const auto before = book.snapshot();
        auto a = short_output ? std::span<Trade>{} : std::span(actual);
        auto b = short_output ? std::span<Trade>{} : std::span(expected);
        const auto want = oracle.apply(e, b), got = book.apply(e, a);
        CHECK(want == got);
        CHECK(std::equal(actual.begin(), actual.begin() + got.trade_count, expected.begin()));
        last.assign(actual.begin(), actual.begin() + got.trade_count);
        const auto state = book.snapshot();
        CHECK(state == oracle.snapshot()); CHECK(book.size() == state.size()); CHECK(book.invariant());
        if(got.status != Status::ok) CHECK(state == before);
        std::optional<Price> bid, ask;
        for(const auto& o : state) {
            if(o.side == Side::buy) bid = bid ? std::max(*bid, o.price) : o.price;
            else ask = ask ? std::min(*ask, o.price) : o.price;
        }
        CHECK(book.best_bid() == bid && book.best_ask() == ask);
        return got;
    }
};
void matching() {
    Harness h;
    h.apply({Kind::add, 1, Side::sell, 101, 10});
    h.apply({Kind::add, 2, Side::sell, 101, 20});
    h.apply({Kind::add, 3, Side::sell, 102, 7});
    CHECK(h.apply({Kind::add, 4, Side::buy, 102, 35}).remaining == 0);
    CHECK((h.last == std::vector<Trade>{{1,4,101,10},{2,4,101,20},{3,4,102,5}}));
    CHECK(h.apply({Kind::match, 0, Side::buy, 0, 10}).remaining == 8);
    CHECK((h.last == std::vector<Trade>{{3,0,102,2}}));
    CHECK(!h.book.best_ask());
    h.apply({Kind::add, 5, Side::buy, 100, 10});
    h.apply({Kind::add, 6, Side::buy, 99, 20});
    CHECK(h.apply({Kind::match, 0, Side::sell, 0, 15}).remaining == 0);
    CHECK((h.last == std::vector<Trade>{{5,0,100,10},{6,0,99,5}}));
    CHECK(h.book.snapshot().front().quantity == 15);
    // Swept IDs may be reused; the old hash entry must have been removed.
    CHECK(h.apply({Kind::add, 5, Side::buy, 98, 3}).status == Status::ok);
    CHECK(h.apply({Kind::cancel, 6}).status == Status::ok);
    CHECK(h.apply({Kind::cancel, 6}).status == Status::not_found);
}
void priority() {
    Harness h;
    h.apply({Kind::add, 1, Side::sell, 101, 10}); h.apply({Kind::add, 2, Side::sell, 101, 10});
    h.apply({Kind::modify, 1, Side::buy, 101, 5}); // Modify ignores event side.
    h.apply({Kind::match, 0, Side::buy, 0, 5});
    CHECK(h.last.front().maker == 1);
    h.apply({Kind::add, 1, Side::sell, 101, 10});
    h.apply({Kind::modify, 2, Side::buy, 101, 10}); // Equal quantity is a no-op.
    CHECK(h.book.snapshot().front().id == 2);
    h.apply({Kind::modify, 2, Side::buy, 101, 20}); // Increase loses priority.
    CHECK(h.book.snapshot().front().id == 1);
    h.apply({Kind::modify, 1, Side::buy, 100, 10});
    h.apply({Kind::match, 0, Side::buy, 0, 15});
    CHECK((h.last == std::vector<Trade>{{1,0,100,10},{2,0,101,5}}));
    h.apply({Kind::add, 3, Side::buy, 99, 20});
    CHECK(h.apply({Kind::modify, 3, Side::sell, 101, 20}).remaining == 5);
    CHECK((h.last == std::vector<Trade>{{2,3,101,15}}));
    CHECK(h.book.best_bid() == 101 && !h.book.best_ask());
    h.apply({Kind::modify, 3, Side::sell, 0, 0}); // Zero cancels, price ignored.
    CHECK(h.book.size() == 0);
}
void bounds() {
    Harness h({10, 20, 2});
    CHECK(h.apply({Kind::add, 0, Side::buy, 10, 1}).status == Status::invalid);
    CHECK(h.apply({Kind::add, 1, Side::buy, 9, 1}).status == Status::invalid);
    CHECK(h.apply({Kind::add, 1, Side::buy, 10, 0}).status == Status::invalid);
    h.apply({Kind::add, 1, Side::buy, 10, 1}); h.apply({Kind::add, 2, Side::buy, 11, 2});
    CHECK(h.apply({Kind::add, 1, Side::buy, 10, 1}).status == Status::duplicate);
    CHECK(h.apply({Kind::add, 3, Side::sell, 10, 100}).status == Status::full);
    CHECK(h.apply({Kind::modify, 1, Side::buy, 12, 5}).status == Status::ok); // Replace at capacity.
    CHECK(h.apply({Kind::modify, 1, Side::buy, 21, 5}).status == Status::invalid);
    CHECK(h.apply({Kind::match, 0, Side::sell, 0, 10}, true).status == Status::output_full);
    CHECK(h.apply({Kind::modify, 1, Side::buy, 13, 10}, true).status == Status::output_full);
    h.apply({Kind::cancel, 2}, true);
    CHECK(h.apply({Kind::add, 2, Side::sell, 20, 1}, true).status == Status::output_full);
    h.apply({Kind::add, 2, Side::sell, 20, 1});
    CHECK(h.apply({Kind::match, 0, static_cast<Side>(3), 0, 1}).status == Status::invalid);
    CHECK(h.apply({static_cast<Kind>(9), 1}).status == Status::invalid);
}
void differential() {
    for(std::uint64_t seed : {0, 42, 9187, 123456}) {
        Harness h({1, 50, 128}); std::mt19937_64 rng(seed);
        for(unsigned i = 0; i < 8000; ++i) {
            Event e{static_cast<Kind>(rng() % 4), rng() % 200,
                    rng() % 2 ? Side::buy : Side::sell, static_cast<Price>(rng() % 52), static_cast<Quantity>(rng() % 50)};
            if(i % 83 == 0) e.side = static_cast<Side>(17);
            h.apply(e, i % 37 == 0);
        }
    }
    for(auto n : {1U, 16U, 256U}) {
        const auto trace = make_trace(n, 300, 718);
        const auto duplicate = make_trace(n, 300, 718);
        CHECK(trace.events == duplicate.events && trace.fingerprint == duplicate.fingerprint);
        CHECK(trace.fingerprint != make_trace(n, 300, 719).fingerprint);
        Harness h(trace.limits);
        for(const auto& event : trace.initial) h.apply(event);
        std::array<unsigned, 4> counts{};
        for(const auto& event : trace.events) { h.apply(event); ++counts[static_cast<unsigned>(event.kind)]; }
        CHECK((counts == std::array<unsigned,4>{180,75,30,15}));
        CHECK(h.book.snapshot() == trace.final_state);
    }
}
void rehash_and_levels() {
    Harness h({1, 200, 3000});
    for(Id id = 1; id <= 2000; ++id) h.apply({Kind::add, id, Side::sell, static_cast<Price>(100 + id % 4), 1});
    for(Id id = 1; id <= 2000; id += 3) h.apply({Kind::cancel, id});
    const auto expected = h.book.snapshot();
    const auto outcome = h.apply({Kind::match, 0, Side::buy, 0, 3000});
    CHECK(outcome.trade_count == expected.size());
    for(std::size_t i = 0; i < expected.size(); ++i) CHECK(h.last[i].maker == expected[i].id);
    CHECK(h.book.size() == 0 && !h.book.best_ask());
}
int main() {
    try {
        static_assert(!std::is_copy_constructible_v<MapBook> && !std::is_move_constructible_v<MapBook>);
        matching(); priority(); bounds(); differential(); rehash_and_levels();
        std::cout << "OrderBook FIFO/cross-price/partial fills/modify/ID reuse/capacity/output bounds; 32000 differential events PASS\n";
    } catch(const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
