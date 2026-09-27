#include "lab/benchmark.hpp"
#include "lab/container_workload.hpp"
#include "lab/layout.hpp"
#include <random>
#include <stdexcept>

namespace lab {
namespace {
template<class Container> void run_container(const Config& c, const std::string& name,
                                            const containers::Workload& w, Results& results) {
    for(const std::string operation : {"lookup", "insert", "iterate", "erase"}) {
        const auto variant = name + "_" + operation;
        if(!selected(c, variant)) continue;
        Container data;
        const bool mutating = operation == "insert" || operation == "erase";
        const auto reset = [&] { data.reset(w.initial, c.size + c.batch); };
        reset();
        auto expected = w.initial;
        if(operation == "insert") expected.insert(expected.end(), w.inserted.begin(), w.inserted.end());
        if(operation == "erase") {
            auto keys = w.erased;
            std::sort(keys.begin(), keys.end());
            std::erase_if(expected, [&](const auto& e) { return std::binary_search(keys.begin(), keys.end(), e.first); });
        }
        std::sort(expected.begin(), expected.end());
        containers::Value checksum = 0;
        std::size_t hits = 0;
        std::function<void()> work;
        if(operation == "lookup") work = [&] {
            checksum = 0; hits = 0;
            for(auto key : w.queries) if(const auto value = data.lookup(key)) { checksum += *value; ++hits; }
            do_not_optimize(checksum); do_not_optimize(hits);
        };
        else if(operation == "insert") work = [&] { for(auto entry : w.inserted) data.insert_new(entry); do_not_optimize(data); };
        else if(operation == "erase") work = [&] { hits = 0; for(auto key : w.erased) hits += data.erase(key); do_not_optimize(hits); };
        else work = [&] { checksum = data.sum(); do_not_optimize(checksum); };
        const auto validate = [&] {
            if(operation == "lookup" && (checksum != w.lookup_sum || hits != w.lookup_hits))
                throw std::runtime_error("container lookup mismatch");
            if(operation == "iterate" && checksum != w.initial_sum)
                throw std::runtime_error("container iteration mismatch");
            if(operation == "erase" && hits != w.erased.size())
                throw std::runtime_error("container erase mismatch");
            if(mutating && data.snapshot() != expected) throw std::runtime_error("container final state mismatch");
        };
        const auto ops = operation == "iterate" ? c.size : operation == "erase" ? w.erased.size() : c.batch;
        auto row = measure(c, "containers", variant, ops, work, "throughput", "batch_mean",
                           mutating ? std::function<void()>(reset) : std::function<void()>(), validate);
        row.metrics["initial_records"] = c.size;
        row.metrics["final_records"] = expected.size();
        row.metrics["record_bytes"] = sizeof(containers::Entry);
        if(operation == "lookup") row.metrics["hit_fraction"] = static_cast<double>(w.lookup_hits) / c.batch;
        row.notes = "size=initial records; identical seeded unique key/value workload. Lookup ~50% hits; erase includes key search; insert uses known-new keys. Vector/hash capacity reserved for N+batch before timing; node allocation remains timed. Mutable state reset and checked outside EVERY sample; warmed/reused capacity. Iterate=full pass, other operations=batch (erase=min(batch,N)). ns/op batch percentiles are not individual latency. Traversal order differs; checksum is order-independent. No cold-cache claim.";
        results.push_back(std::move(row));
    }
}
} // namespace

Results containers_bench(const Config& c) {
    require_threads(c, 1);
    const auto w = containers::workload(c.size, c.batch, c.seed);
    Results rows;
    run_container<containers::Vector>(c, "vector", w, rows);
    run_container<containers::List>(c, "list", w, rows);
    run_container<containers::Deque>(c, "deque", w, rows);
    run_container<containers::Map>(c, "map", w, rows);
    run_container<containers::UnorderedMap>(c, "unordered_map", w, rows);
    run_container<containers::SortedVector>(c, "sorted_vector", w, rows);
    return rows;
}

Results data_layout(const Config& c) {
    require_threads(c, 1);
    if(c.size == 0 || c.size > 1000000) throw std::invalid_argument("data_layout: size 1..1000000 records");
    Results rows;
    for(const std::string variant : {"aos", "soa"}) {
        if(!selected(c, variant)) continue;
        std::mt19937_64 rng(c.seed);
        std::uint64_t expected = 0, checksum = 0;
        std::vector<layout::Order> aos;
        layout::SoA soa;
        if(variant == "aos") aos.resize(c.size);
        else { soa.price.resize(c.size); soa.quantity.resize(c.size); soa.id.resize(c.size); soa.side.resize(c.size); }
        for(std::size_t i = 0; i < c.size; ++i) {
            const std::uint64_t price = 1 + rng() % 1000000;
            const auto quantity = static_cast<std::uint32_t>(1 + rng() % 1000);
            const auto id = static_cast<std::uint32_t>(i);
            const char side = i % 2 == 0 ? 'B' : 'S';
            expected += price * quantity;
            if(variant == "aos") aos[i] = {price, quantity, id, side};
            else { soa.price[i] = price; soa.quantity[i] = quantity; soa.id[i] = id; soa.side[i] = side; }
        }
        std::function<void()> work;
        if(variant == "aos") work = [&] { checksum = layout::sum_aos(aos.data(), aos.size()); do_not_optimize(checksum); };
        else work = [&] { checksum = layout::sum_soa(soa.price.data(), soa.quantity.data(), c.size); do_not_optimize(checksum); };
        auto row = measure(c, "data_layout", variant, c.size, work, "throughput", "full_pass_mean", {}, [&] {
            if(checksum != expected) throw std::runtime_error("layout checksum mismatch");
        });
        row.metrics["records"] = c.size;
        row.metrics["checksum"] = static_cast<double>(checksum);
        row.metrics["useful_bytes_per_record"] = sizeof(std::uint64_t) + sizeof(std::uint32_t);
        row.metrics["logical_storage_bytes"] = c.size * (variant == "aos" ? sizeof(layout::Order) : sizeof(std::uint64_t) + 2 * sizeof(std::uint32_t) + sizeof(char));
        row.metrics["useful_bytes_per_second"] = row.total_operations * 12 * 1e9 / row.total_ns;
        row.notes = "size=records; batch unused; full price*quantity reduction/pass, integer ticks with exact same arithmetic and data. id/side stored but not scanned. Setup, allocation, validation excluded. Useful bytes/sec is logical field traffic, not DRAM bandwidth. Scalar/vectorization depends on actual compiler flags; no fast-math. Full-pass mean tails are not per-order latency.";
        rows.push_back(std::move(row));
    }
    return rows;
}
} // namespace lab
