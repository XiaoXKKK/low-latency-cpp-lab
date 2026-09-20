#include "lab/benchmark.hpp"
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
namespace lab {
namespace {
template<class Mutex,bool Shared> Result run(const Config& c,const std::string& variant) {
    Mutex mutex; std::uint64_t writes=0,state=c.seed;
    std::vector<std::uint64_t> checksums(c.threads);
    auto r=measure_parallel(c,"rw_locks",variant,c.batch*c.threads,[&](unsigned thread){
        std::uint64_t sum=0;
        for(std::size_t i=0;i<c.batch;++i) {
            if((i+thread)%100<c.read_percent) {
                if constexpr(Shared) { std::shared_lock lock(mutex); sum+=state; }
                else { std::lock_guard lock(mutex); sum+=state; }
            } else {
                std::lock_guard lock(mutex); ++writes;
                for(std::size_t step=0;step<c.critical;++step) state=state*6364136223846793005ULL+1;
            }
        }
        checksums[thread]=sum; do_not_optimize(sum);
    });
    std::uint64_t expected_per_sample=0;
    for(unsigned t=0;t<c.threads;++t) for(std::size_t i=0;i<c.batch;++i) if((i+t)%100>=c.read_percent) ++expected_per_sample;
    if(writes!=expected_per_sample*(c.warmup+r.samples.size())) throw std::runtime_error("rw_locks lost write");
    r.metrics["write_operations_per_sample"]=static_cast<double>(expected_per_sample);
    r.notes+=" Identical deterministic read/write mix; --read-percent controls shared reads; --critical writer steps. Reader/writer fairness is implementation-specific. Throughput cannot establish writer tail latency.";
    return r;
}
}
Results rw_locks(const Config& c) {
    Results rows;
    if(selected(c,"mutex")) rows.push_back(run<std::mutex,false>(c,"mutex"));
    if(selected(c,"shared_mutex")) rows.push_back(run<std::shared_mutex,true>(c,"shared_mutex"));
    return rows;
}
}
