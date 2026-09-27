#pragma once
#include <algorithm>
#include <cstdint>
#include <deque>
#include <list>
#include <map>
#include <optional>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace lab::containers {
using Key = std::uint64_t;
using Value = std::uint64_t;
using Entry = std::pair<Key, Value>;
inline Value value_for(Key key) { return key * 17 + 3; }

// Unique keys, explicit 50% hit queries, and successful erases. Workload creation
// and resetting the initial N records are outside every timed sample.
struct Workload {
    std::vector<Entry> initial, inserted;
    std::vector<Key> queries, erased;
    Value lookup_sum = 0, initial_sum = 0;
    std::size_t lookup_hits = 0;
};
inline Workload workload(std::size_t n, std::size_t batch, std::uint64_t seed) {
    if(n == 0 || n > 100000 || batch == 0 || batch > 4096)
        throw std::invalid_argument("containers: size 1..100000 records; batch 1..4096");
    Workload w;
    std::mt19937_64 rng(seed);
    for(std::size_t i = 0; i < n; ++i) {
        w.initial.emplace_back(2 * i, value_for(2 * i));
        w.initial_sum += value_for(2 * i);
    }
    std::shuffle(w.initial.begin(), w.initial.end(), rng);
    for(std::size_t i = 0; i < batch; ++i) {
        const Key key = 2 * (rng() % n) + i % 2;
        w.queries.push_back(key);
        if(key % 2 == 0) { ++w.lookup_hits; w.lookup_sum += value_for(key); }
        // Spread fresh odd keys across the initial even-key range when batch
        // <= N; above N, append the remaining unique odd keys beyond that range.
        const Key new_key = 2 * (batch <= n ? i * n / batch : i) + 1;
        w.inserted.emplace_back(new_key, value_for(new_key));
    }
    // Random insertion ranks for the sorted container, with identical keys for
    // every adapter. Existing keys and inserted keys never overlap.
    std::shuffle(w.inserted.begin(), w.inserted.end(), rng);
    for(const auto& entry : w.initial) w.erased.push_back(entry.first);
    std::shuffle(w.erased.begin(), w.erased.end(), rng);
    w.erased.resize(std::min(n, batch));
    std::shuffle(w.queries.begin(), w.queries.end(), rng);
    return w;
}

template<class Storage, bool Sorted = false> class Sequence {
    Storage data_;
    auto locate(Key key) {
        if constexpr(Sorted) return std::lower_bound(data_.begin(), data_.end(), key,
            [](const Entry& entry, Key k) { return entry.first < k; });
        else return std::find_if(data_.begin(), data_.end(), [key](const Entry& e) { return e.first == key; });
    }
public:
    void reset(const std::vector<Entry>& entries, std::size_t capacity) {
        data_.clear();
        if constexpr(requires { data_.reserve(capacity); }) data_.reserve(capacity);
        data_.insert(data_.end(), entries.begin(), entries.end());
        if constexpr(Sorted) std::sort(data_.begin(), data_.end());
    }
    std::optional<Value> lookup(Key key) {
        auto it = locate(key);
        if(it == data_.end() || it->first != key) return std::nullopt;
        return it->second;
    }
    // Caller guarantees a new key; sequence insertion does not perform a
    // redundant uniqueness search. Arbitrary-key erase includes key lookup.
    void insert_new(Entry entry) {
        if constexpr(Sorted) data_.insert(locate(entry.first), entry);
        else data_.push_back(entry);
    }
    bool erase(Key key) {
        auto it = locate(key);
        if(it == data_.end() || it->first != key) return false;
        data_.erase(it);
        return true;
    }
    Value sum() const { Value total = 0; for(const auto& e : data_) total += e.second; return total; }
    std::vector<Entry> snapshot() const {
        std::vector<Entry> result(data_.begin(), data_.end());
        std::sort(result.begin(), result.end());
        return result;
    }
};

template<class Storage> class Associative {
    Storage data_;
public:
    void reset(const std::vector<Entry>& entries, std::size_t capacity) {
        data_.clear();
        if constexpr(requires { data_.reserve(capacity); }) data_.reserve(capacity);
        data_.insert(entries.begin(), entries.end());
    }
    std::optional<Value> lookup(Key key) const {
        const auto it = data_.find(key);
        if(it == data_.end()) return std::nullopt;
        return it->second;
    }
    void insert_new(Entry entry) { data_.insert(entry); }
    bool erase(Key key) { return data_.erase(key) != 0; }
    Value sum() const { Value total = 0; for(const auto& e : data_) total += e.second; return total; }
    std::vector<Entry> snapshot() const {
        std::vector<Entry> result(data_.begin(), data_.end());
        std::sort(result.begin(), result.end());
        return result;
    }
};
using Vector = Sequence<std::vector<Entry>>;
using List = Sequence<std::list<Entry>>;
using Deque = Sequence<std::deque<Entry>>;
using SortedVector = Sequence<std::vector<Entry>, true>;
using Map = Associative<std::map<Key, Value>>;
using UnorderedMap = Associative<std::unordered_map<Key, Value>>;
} // namespace lab::containers
