#include "lab/benchmark.hpp"
#include <stdexcept>
namespace lab {
Results atomic_order(const Config& c) {
    require_threads(c,1); Results rows;
    auto rmw=[&]<std::memory_order Order>(const std::string& variant) {
        if(!selected(c,variant)) return;
        std::atomic<std::uint64_t> value{0};
        auto r=measure(c,"atomic_order",variant,c.batch,[&]{
            for(std::size_t i=0;i<c.batch;++i) value.fetch_add(1,Order);
            do_not_optimize(value.load(std::memory_order_relaxed));
        });
        if(value.load()!=(c.warmup+r.samples.size())*c.batch) throw std::runtime_error("atomic count mismatch");
        r.notes="Single-thread atomic RMW; order is compile-time constant. Same x86 LOCK instruction may result for different C++ orders. Does not test inter-thread publication correctness.";
        rows.push_back(std::move(r));
    };
    rmw.template operator()<std::memory_order_relaxed>("relaxed_rmw");
    rmw.template operator()<std::memory_order_acquire>("acquire_rmw");
    rmw.template operator()<std::memory_order_release>("release_rmw");
    rmw.template operator()<std::memory_order_acq_rel>("acq_rel_rmw");
    rmw.template operator()<std::memory_order_seq_cst>("seq_cst_rmw");
    auto stores=[&]<std::memory_order Order>(const std::string& variant) {
        if(!selected(c,variant)) return;
        std::atomic<std::uint64_t> value{0};
        auto r=measure(c,"atomic_order",variant,c.batch,[&]{
            for(std::size_t i=0;i<c.batch;++i) value.store(i,Order);
            do_not_optimize(value.load(std::memory_order_relaxed));
        });
        if(value.load()!=c.batch-1) throw std::runtime_error("atomic store mismatch");
        r.notes="Single-thread stores; compile-time legal store orders only. Compare stores with stores, RMW with RMW. Identical runtime never proves relaxed publication safe.";
        rows.push_back(std::move(r));
    };
    stores.template operator()<std::memory_order_relaxed>("relaxed_store");
    stores.template operator()<std::memory_order_release>("release_store");
    stores.template operator()<std::memory_order_seq_cst>("seq_cst_store");
    return rows;
}
}
