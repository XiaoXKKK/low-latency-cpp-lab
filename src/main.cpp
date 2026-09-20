#include "lab/benchmark.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        if(argc==2 && std::string(argv[1])=="--help") {
            std::cout<<"lab_bench --benchmark timer|memory_access|false_sharing|branch|allocation|locks|spsc|affinity\n"
                       "tsc_interval|perf_interval|cache_patterns|allocator_batch|allocation_handoff|page_behavior\n"
                       "atomic_order|rw_locks|wait_strategy|arrival_latency|numa_access\n"
                       "--iterations 100 --warmup 10 --threads 1 --cpu -1|0,1 --duration 0\n"
                       "--batch 4096 --size 32768 --seed 42 --critical 0 --locks 1\n"
                       "--stride 1 --distance 16 --interval-ns 100000 --read-percent 90\n"
                       "--memory-node -1 --touch-cpu -1 --background-cpu -1\n"
                       "--variant NAME --format table|json|csv\n"
                       "iterations = measured samples; warmup = discarded samples; duration = per-result wall-time cap (seconds).\n"
                       "Workers reuse cpu list cyclically; one CPU means intentional oversubscription.\n";
            return 0;
        }
        auto c=lab::parse(argc,argv);
        const std::map<std::string,lab::Results(*)(const lab::Config&)> registry={
            {"timer",lab::timer},{"memory_access",lab::memory_access},{"false_sharing",lab::false_sharing},
            {"branch",lab::branch},{"allocation",lab::allocation},{"locks",lab::locks},{"spsc",lab::spsc},{"affinity",lab::affinity},
            {"tsc_interval",lab::tsc_interval},{"perf_interval",lab::perf_interval},
            {"cache_patterns",lab::cache_patterns},{"allocator_batch",lab::allocator_batch},
            {"allocation_handoff",lab::allocation_handoff},{"page_behavior",lab::page_behavior},
            {"atomic_order",lab::atomic_order},{"rw_locks",lab::rw_locks},
            {"wait_strategy",lab::wait_strategy},{"arrival_latency",lab::arrival_latency},
            {"numa_access",lab::numa_access}};
        auto it=registry.find(c.benchmark);
        if(it==registry.end()) throw std::invalid_argument("unknown benchmark");
        lab::AffinityGuard restore;
        if(c.threads==1 && !c.cpus.empty() && c.benchmark!="affinity" && c.benchmark!="arrival_latency" && c.benchmark!="numa_access") lab::pin_current(c.cpus.front());
        auto results=it->second(c);
        if(results.empty()) throw std::invalid_argument("unknown or unsupported variant");
        lab::output(c,results);
    } catch(const std::exception& e) { std::cerr<<"ERROR: "<<e.what()<<'\n'; return 2; }
}
