#include "lab/benchmark.hpp"
#include "lab/structures.hpp"
#include "lab/locks.hpp"
#include <memory>
#include <mutex>
#include <stdexcept>
namespace lab {
template<class Lock> Result lock_run(const Config& c,const std::string& name) {
    struct alignas(128) Cell { Lock lock; std::uint64_t count=0, state=1; };
    auto cells=std::make_unique<Cell[]>(c.locks);
    auto r=measure_parallel(c,"locks",name,c.batch*c.threads,[&](unsigned t) {
        auto& cell=cells[t%c.locks];
        for(std::size_t i=0;i<c.batch;++i) {
            std::lock_guard guard(cell.lock); ++cell.count;
            for(std::size_t j=0;j<c.critical;++j) cell.state=cell.state*6364136223846793005ULL+1;
            do_not_optimize(cell.state);
        }
    });
    std::uint64_t total=0; for(std::size_t i=0;i<c.locks;++i) total+=cells[i].count;
    if(total!=(c.warmup+r.samples.size())*c.batch*c.threads) throw std::runtime_error("lost lock updates");
    r.notes+=" --locks partitions contention (1=shared; >=threads=private). --critical counts dependent integer updates inside lock. No fairness guarantee for spinlock.";
    return r;
}
Results locks(const Config& c) {
    Results rows;
    if(selected(c,"mutex")) rows.push_back(lock_run<std::mutex>(c,"mutex"));
    if(selected(c,"spinlock")) rows.push_back(lock_run<SpinLock>(c,"spinlock"));
    if(selected(c,"ticket")) rows.push_back(lock_run<TicketLock>(c,"ticket"));
    return rows;
}
}
