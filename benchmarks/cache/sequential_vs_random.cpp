#include "lab/benchmark.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>
namespace lab {
Results memory_access(const Config& c) {
    require_threads(c,1);
    if(c.size<2*sizeof(std::uint32_t) || c.size%sizeof(std::uint32_t)) throw std::invalid_argument("size must be multiple of 4, >= 8 bytes");
    const auto n=c.size/sizeof(std::uint32_t);
    std::vector<std::uint32_t> next(n), permutation(n);
    std::iota(permutation.begin(),permutation.end(),0U);
    std::mt19937_64 random(c.seed); Results rows;
    for(const std::string variant:{"sequential","random"}) {
        if(!selected(c,variant)) continue;
        if(variant=="random") std::shuffle(permutation.begin(),permutation.end(),random);
        for(std::size_t i=0;i<n;++i) next[permutation[i]]=permutation[(i+1)%n];
        std::uint32_t cursor=0;
        // Traverse one complete cycle before warmup: allocate/fault outside timing.
        for(std::size_t i=0;i<n;++i) cursor=next[cursor];
        const auto operations=std::max(c.batch,n);
        auto r=measure(c,"memory_access",variant,operations,[&] {
            for(std::size_t i=0;i<operations;++i) cursor=next[cursor];
            do_not_optimize(cursor);
        },"latency","batch_mean_dependent_load");
        r.notes="Dependent pointer chase; both variants use a single full permutation cycle. Prefaulted; each sample visits at least the full working set. Large sets need not fit cache. ns/dependent load, not bandwidth. Cursor continues across samples.";
        rows.push_back(std::move(r));
    }
    return rows;
}
}
