#include "lab/benchmark.hpp"
#include <algorithm>
#include <random>
namespace lab {
__attribute__((noinline)) std::uint64_t branch_kernel(const std::uint32_t* data,std::size_t n) {
    std::uint64_t sum=0;
    for(std::size_t i=0;i<n;++i) {
        if(data[i]>127) {
            // Observable compiler barrier prevents if-conversion of this arm.
            asm volatile("" : "+r"(sum) : : "memory");
            sum+=data[i];
        }
    }
    return sum;
}
__attribute__((noinline)) std::uint64_t branchless_kernel(const std::uint32_t* data,std::size_t n) {
    std::uint64_t sum=0;
    for(std::size_t i=0;i<n;++i) sum+=data[i]*static_cast<std::uint32_t>(data[i]>127);
    return sum;
}
Results branch(const Config& c) {
    require_threads(c,1); std::mt19937_64 rng(c.seed);
    std::vector<std::uint32_t> random(c.batch);
    for(auto& x:random) x=static_cast<std::uint32_t>(rng() & 255U);
    auto sorted=random; std::sort(sorted.begin(),sorted.end()); Results rows;
    for(const std::string variant:{"sorted_branch","random_branch","sorted_branchless","random_branchless"}) {
        if(!selected(c,variant)) continue;
        const auto& data=variant.starts_with("sorted")?sorted:random;
        const bool branchless=variant.ends_with("branchless");
        auto r=measure(c,"branch",variant,data.size(),[&] {
            compiler_barrier(); auto sum=branchless?branchless_kernel(data.data(),data.size()):branch_kernel(data.data(),data.size());
            do_not_optimize(sum);
        });
        r.notes="Identical values, sorted vs shuffled order. Scalar kernels; branch arm has compiler-only barrier. Inspect disassembly; short repeated random patterns may be learned.";
        rows.push_back(std::move(r));
    }
    return rows;
}
}
