#include "lab/benchmark.hpp"
#include <ctime>
#include <stdexcept>
#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#include <x86intrin.h>
#endif
namespace lab {
Results timer(const Config& c) {
    require_threads(c,1); Results rows;
    auto add=[&](const std::string& name,auto read) {
        if(!selected(c,name)) return;
        auto r=measure(c,"timer",name,c.batch,[&] {
            for(std::size_t i=0;i<c.batch;++i) { auto v=read(); do_not_optimize(v); }
        });
        r.notes="Outer steady_clock measurement; ns per timer read including loop/barrier. TSC values are ticks, not core cycles.";
        rows.push_back(std::move(r));
    };
    add("empty_loop",[]{compiler_barrier(); return 0;});
    add("steady_clock",[]{return Clock::now().time_since_epoch().count();});
    add("clock_gettime",[]{timespec ts{}; if(clock_gettime(CLOCK_MONOTONIC,&ts)!=0) throw std::runtime_error("clock_gettime"); return ts.tv_nsec;});
#if defined(__x86_64__) || defined(__i386__)
    unsigned a=0,b=0,ecx=0,d=0;
    __get_cpuid(1,&a,&b,&ecx,&d);
    if(d & (1U<<4)) {
        add("rdtsc_raw",[]{return __rdtsc();});
        add("lfence_rdtsc",[]{_mm_lfence(); auto t=__rdtsc(); _mm_lfence(); return t;});
    }
    if(__get_cpuid(0x80000001,&a,&b,&ecx,&d) && (d & (1U<<27)))
        add("rdtscp_lfence",[]{unsigned aux=0; auto t=__rdtscp(&aux); _mm_lfence(); do_not_optimize(aux); return t;});
#endif
    return rows;
}
}
