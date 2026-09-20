#include "lab/benchmark.hpp"
#include "lab/linux_memory.hpp"
#include <algorithm>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <sys/mman.h>
namespace lab {
Results numa_access(const Config& c) {
    require_threads(c,1);
    if(c.size<page_size() || c.size%page_size()) throw std::invalid_argument("NUMA size must be page multiple");
    const auto allowed=allowed_cpus();
    const int cpu=c.cpus.empty()?allowed.front():c.cpus.front();
    Results rows;
    for(const std::string variant:{"bound_streaming","bound_random","first_touch_streaming","first_touch_random"}) {
        if(!selected(c,variant)) continue;
        try {
            const int local_node=cpu_node(cpu), target_node=c.memory_node<0?local_node:c.memory_node;
            int touch_cpu=c.touch_cpu;
            if(touch_cpu<0 && variant.starts_with("bound")) touch_cpu=cpu;
            if(touch_cpu<0) {
                for(int candidate:allowed) if(cpu_node(candidate)==target_node) { touch_cpu=candidate; break; }
            }
            if(touch_cpu<0) throw std::runtime_error("No allowed first-touch CPU on target node");
            AffinityGuard restore;
            MappedRegion region(c.size); region.advise(MADV_NOHUGEPAGE);
            if(variant.starts_with("bound")) bind_memory(region,target_node);
            pin_current(touch_cpu);
            const auto n=region.size()/sizeof(std::uint32_t);
            auto* data=static_cast<std::uint32_t*>(region.data());
            for(std::size_t i=0;i<n;++i) std::construct_at(data+i,static_cast<std::uint32_t>((i+1)%n));
            if(variant.ends_with("random")) {
                std::vector<std::uint32_t> order(n); std::iota(order.begin(),order.end(),0U);
                std::mt19937_64 random(c.seed); std::shuffle(order.begin(),order.end(),random);
                for(std::size_t i=0;i<n;++i) data[order[i]]=order[(i+1)%n];
            }
            pin_current(cpu);
            auto verify=[&]{
                auto nodes=page_nodes(region);
                if(!std::all_of(nodes.begin(),nodes.end(),[&](int node){return node==target_node;}))
                    throw std::runtime_error("Actual pages not all on requested node; locality comparison rejected");
                return nodes.size();
            };
            const auto pages=verify(); std::uint32_t cursor=0;
            const bool chase=variant.ends_with("random");
            auto r=measure(c,"numa_access",variant,n,[&]{
                if(chase) { for(std::size_t i=0;i<n;++i) cursor=data[cursor]; do_not_optimize(cursor); }
                else { std::uint64_t sum=0; for(std::size_t i=0;i<n;++i) sum+=data[i]; do_not_optimize(sum); }
            },chase?"latency":"throughput",chase?"batch_mean_dependent_load":"batch_mean_streaming_load");
            verify();
            r.metrics={{"cpu",static_cast<double>(cpu)},{"cpu_node",static_cast<double>(local_node)},
                {"memory_node",static_cast<double>(target_node)},{"touch_cpu",static_cast<double>(touch_cpu)},
                {"verified_base_pages",static_cast<double>(pages)},{"remote",target_node!=local_node?1.0:0.0}};
            if(!chase) r.metrics["useful_bytes_per_second"]=r.total_operations*4*1e9/r.total_ns;
            r.notes="Actual placement of every base page verified via move_pages before/after; reject mixed/changed/unqueryable placement. Full pass/sample; --batch unused. First touch and policy binding are separate variants. Snapshot checks cannot rule out temporary mid-run migration. Bound policy uses mbind syscall (no libnuma dependency).";
            rows.push_back(std::move(r));
        } catch(const std::runtime_error& e) { rows.push_back(unavailable(c,"numa_access",variant,e.what())); }
    }
    return rows;
}
}
