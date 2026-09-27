#include "lab/container_workload.hpp"
#include "lab/layout.hpp"
#include <iostream>
#include <string>
#define CHECK(value) do { if(!(value)) throw std::runtime_error(#value); } while(false)
using namespace lab::containers;

template<class Container> void check_container() {
    Container data;
    // A simple key-indexed oracle is independent of all six adapters.
    std::vector<std::optional<Value>> oracle(1000);
    data.reset({}, 1000);
    std::mt19937_64 rng(718);
    for(int step = 0; step < 4000; ++step) {
        const auto key = rng() % oracle.size();
        switch(rng() % 3) {
        case 0:
            if(!oracle[key]) { const auto value = rng(); data.insert_new({key, value}); oracle[key] = value; }
            break;
        case 1:
            CHECK(data.erase(key) == oracle[key].has_value()); oracle[key].reset();
            break;
        default: CHECK(data.lookup(key) == oracle[key]); break;
        }
        if(step % 31 == 0) {
            std::vector<Entry> expected;
            Value total = 0;
            for(std::size_t i = 0; i < oracle.size(); ++i) if(oracle[i]) { expected.emplace_back(i, *oracle[i]); total += *oracle[i]; }
            CHECK(data.snapshot() == expected);
            CHECK(data.sum() == total);
        }
    }
    for(auto n : {1U, 16U, 64U, 256U, 1024U, 10000U, 100000U}) {
        auto w = workload(n, 17, 42);
        data.reset(w.initial, n + w.inserted.size());
        CHECK(data.sum() == w.initial_sum);
        Value total = 0; std::size_t hits = 0;
        for(auto key : w.queries) if(auto value = data.lookup(key)) { total += *value; ++hits; }
        CHECK(total == w.lookup_sum && hits == w.lookup_hits);
        for(auto key : w.erased) { CHECK(data.erase(key)); CHECK(!data.lookup(key)); CHECK(!data.erase(key)); }
        for(auto entry : w.inserted) { data.insert_new(entry); CHECK(data.lookup(entry.first) == entry.second); }
        data.reset({}, 0); CHECK(data.snapshot().empty()); CHECK(!data.lookup(0));
    }
}
int main() {
    try {
        check_container<Vector>(); check_container<List>(); check_container<Deque>();
        check_container<Map>(); check_container<UnorderedMap>(); check_container<SortedVector>();
        CHECK(lab::layout::sum_aos(nullptr, 0) == 0);
        CHECK(lab::layout::sum_soa(nullptr, nullptr, 0) == 0);
        std::mt19937_64 rng(27);
        for(auto n : {1U, 3U, 16U, 31U, 64U, 1025U}) {
            std::vector<lab::layout::Order> aos;
            lab::layout::SoA soa;
            std::uint64_t expected = 0;
            for(unsigned i = 0; i < n; ++i) {
                const auto p = rng(); const auto q = static_cast<std::uint32_t>(rng());
                expected += p * q; aos.push_back({p, q, i, 'B'});
                soa.price.push_back(p); soa.quantity.push_back(q);
            }
            CHECK(lab::layout::sum_aos(aos.data(), n) == expected);
            CHECK(lab::layout::sum_soa(soa.price.data(), soa.quantity.data(), n) == expected);
        }
        std::cout << "containers: randomized oracle, hit/miss, erase/reset, all sizes; layout exact reductions PASS\n";
    } catch(const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
