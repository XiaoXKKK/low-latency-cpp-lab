#include "lab/benchmark.hpp"
#include <sched.h>
namespace lab {
Results affinity(const Config& c) {
    require_threads(c,1); Results rows;
    const auto cpus=allowed_cpus();
    for(const std::string variant:{"unpinned","pinned"}) {
        if(!selected(c,variant)) continue;
        AffinityGuard restore;
        if(variant=="pinned") pin_current(c.cpus.empty()?cpus.front():c.cpus.front());
        std::uint64_t state=c.seed, migrations=0;
        int previous=sched_getcpu();
        auto r=measure(c,"affinity",variant,1,[&] {
            int current=sched_getcpu(); if(current!=previous) ++migrations; previous=current;
            for(std::size_t i=0;i<c.batch;++i) state=state*6364136223846793005ULL+1;
            do_not_optimize(state);
        },"latency","individual_work_unit");
        r.notes="ns per work unit (--batch dependent arithmetic steps), includes sched_getcpu. Observed CPU changes including warmup="+std::to_string(migrations)+
                "; endpoint sampling can miss migrations. Unpinned preserves inherited allowed mask; pinned CPU="+std::to_string(c.cpus.empty()?cpus.front():c.cpus.front());
        rows.push_back(std::move(r));
    }
    return rows;
}
}
