#include "lab/benchmark.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>
namespace lab {
namespace {
template<bool Prefetch> std::uint64_t indexed_sum(const std::uint64_t* data,const std::size_t* index,std::size_t n,std::size_t distance) {
    std::uint64_t sum=0;
    for(std::size_t i=0;i<n;++i) {
        if constexpr(Prefetch) if(distance<n-i) __builtin_prefetch(data+index[i+distance],0,1);
        sum+=data[index[i]];
    }
    return sum;
}
}
Results cache_patterns(const Config& c) {
    require_threads(c,1);
    if(c.size<128 || c.size%8) throw std::invalid_argument("cache_patterns size >=128 and multiple of 8");
    const auto n=c.size/8;
    if(c.stride>n) throw std::invalid_argument("stride exceeds working set");
    std::vector<std::uint64_t> data(n); std::vector<std::size_t> index(n);
    std::mt19937_64 random(c.seed); for(auto& x:data) x=random();
    std::iota(index.begin(),index.end(),0); std::shuffle(index.begin(),index.end(),random);
    const auto expected=std::accumulate(data.begin(),data.end(),std::uint64_t{0});
    Results rows;
    for(const std::string variant:{"streaming","stride","indexed","prefetch"}) {
        if(!selected(c,variant)) continue;
        const auto accesses=variant=="stride"?(n+c.stride-1)/c.stride:n;
        std::uint64_t checksum=0;
        std::function<void()> work;
        if(variant=="streaming") work=[&]{ std::uint64_t sum=0; for(auto x:data) sum+=x; checksum=sum; do_not_optimize(sum); };
        else if(variant=="stride") work=[&]{ std::uint64_t sum=0; for(std::size_t i=0;i<n;i+=c.stride) sum+=data[i]; checksum=sum; do_not_optimize(sum); };
        else if(variant=="indexed") work=[&]{ checksum=indexed_sum<false>(data.data(),index.data(),n,c.distance); do_not_optimize(checksum); };
        else work=[&]{ checksum=indexed_sum<true>(data.data(),index.data(),n,c.distance); do_not_optimize(checksum); };
        auto r=measure(c,"cache_patterns",variant,accesses,work);
        if(variant!="stride" && checksum!=expected) throw std::runtime_error("cache checksum mismatch");
        r.metrics["useful_bytes_per_second"]=r.total_operations*8*1e9/r.total_ns;
        r.metrics["data_bytes"]=c.size;
        r.metrics["index_bytes"]=(variant=="indexed" || variant=="prefetch")?index.size()*sizeof(std::size_t):0;
        r.notes="One complete pass/sample; --batch unused. ns/access; useful-byte rate is not DRAM traffic. Indexed variants share same permutation and extra index working set; prefetch uses --distance accesses ahead. Stride visits offsets 0,stride,... without modulo wrap.";
        rows.push_back(std::move(r));
    }
    return rows;
}
}
