#include "lab/benchmark.hpp"
#include <algorithm>
#include <exception>
#include <future>
#include <stdexcept>
#include <thread>
#include <time.h>
namespace lab {
namespace {
double thread_cpu_ns() {
    timespec time{}; if(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&time)) throw std::runtime_error("thread CPU clock");
    return static_cast<double>(time.tv_sec)*1e9+time.tv_nsec;
}
void wait_until(Clock::time_point target,const std::string& strategy) {
    if(strategy=="sleep") std::this_thread::sleep_until(target);
    else if(strategy=="hybrid") {
        const auto tail=std::chrono::microseconds(20);
        if(Clock::now()+tail<target) std::this_thread::sleep_until(target-tail);
        while(Clock::now()<target) pause_cpu();
    } else if(strategy=="yield") { while(Clock::now()<target) std::this_thread::yield(); }
    else { while(Clock::now()<target) pause_cpu(); }
}
// Owns one finite-lifetime background worker. Readiness exceptions propagate before sampling.
class Background {
    std::jthread worker_;
public:
    Background(const std::string& type,int cpu,std::size_t bytes) {
        if(type=="idle") return;
        std::promise<void> ready; auto future=ready.get_future();
        worker_=std::jthread([type,cpu,bytes,ready=std::move(ready)](std::stop_token stop) mutable {
            try {
                pin_current(cpu);
                std::vector<std::uint64_t> memory(type=="memory"?std::max<std::size_t>(bytes/8,1):0,1);
                std::uint64_t value=1; ready.set_value();
                while(!stop.stop_requested()) {
                    if(type=="memory") {
                        for(auto& x:memory) { x=x*6364136223846793005ULL+1; value+=x; }
                        do_not_optimize(memory.data());
                    } else for(std::size_t i=0;i<4096;++i) value=value*6364136223846793005ULL+1;
                    do_not_optimize(value);
                }
            } catch(...) { try { ready.set_exception(std::current_exception()); } catch(...) {} }
        });
        future.get();
    }
};
Result latency_result(const Config& c,const std::string& benchmark,const std::string& variant,const std::string& kind) {
    Result r; r.benchmark=benchmark;r.variant=variant;r.mode="latency";r.sample_kind=kind;r.threads=c.threads;r.samples.reserve(c.iterations);return r;
}
}
Results wait_strategy(const Config& c) {
    require_threads(c,1); Results rows;
    for(const std::string strategy:{"sleep","yield","spin","hybrid"}) {
        if(!selected(c,strategy)) continue;
        auto r=latency_result(c,"wait_strategy",strategy,"individual_wakeup_lateness");
        const auto interval=std::chrono::nanoseconds(c.interval_ns);
        for(std::size_t i=0;i<c.warmup;++i) wait_until(Clock::now()+interval,strategy);
        const auto begin=Clock::now(); const double cpu_begin=thread_cpu_ns();
        for(std::size_t i=0;i<c.iterations;++i) {
            auto target=Clock::now()+interval;
            wait_until(target,strategy); auto end=Clock::now();
            r.samples.push_back(std::max(0.0,elapsed_ns(target,end)));
            if(c.duration>0 && std::chrono::duration<double>(end-begin).count()>=c.duration) break;
        }
        const auto cpu=thread_cpu_ns()-cpu_begin, wall=elapsed_ns(begin,Clock::now());
        r.metrics={{"thread_cpu_ns",cpu},{"measurement_wall_ns",wall},{"thread_cpu_fraction",cpu/wall}};
        r.notes="ns lateness after target wakeup, not total wait duration. Each target is now+interval (closed-loop wait comparison). CPU clock sampled outside loop. Hybrid sleeps until 20us before target, then polls clock/spins; no CPU isolation.";
        rows.push_back(std::move(r));
    }
    return rows;
}
Results arrival_latency(const Config& c) {
    require_threads(c,1); Results rows;
    const auto allowed=allowed_cpus(); const int main_cpu=c.cpus.empty()?allowed.front():c.cpus.front();
    const int background_cpu=c.background_cpu<0?main_cpu:c.background_cpu;
    // Preserve inherited mask before pinning; background can select an explicitly allowed different core.
    for(const std::string variant:{"closed_idle","open_idle","open_cpu","open_memory"}) {
        if(!selected(c,variant)) continue;
        const std::string type=variant=="open_cpu"?"cpu":variant=="open_memory"?"memory":"idle";
        Background background(type,background_cpu,c.size);
        AffinityGuard restore; pin_current(main_cpu);
        std::uint64_t state=c.seed;
        auto service=[&]{for(std::size_t j=0;j<c.batch;++j) state=state*6364136223846793005ULL+1;do_not_optimize(state);};
        for(std::size_t i=0;i<c.warmup;++i) service();
        auto response=latency_result(c,"arrival_latency",variant,"individual_scheduled_response");
        auto service_time=latency_result(c,"arrival_latency",variant+"_service","individual_service_time");
        const auto interval=std::chrono::nanoseconds(c.interval_ns);
        const auto begin=Clock::now(); const double cpu_begin=thread_cpu_ns(); auto scheduled=begin+interval;
        double maximum_backlog=0;
        for(std::size_t i=0;i<c.iterations;++i) {
            if(variant=="closed_idle") scheduled=Clock::now()+interval;
            // Open schedule stays fixed even when overloaded: no late scheduled arrivals are dropped.
            wait_until(scheduled,"spin"); auto start=Clock::now(); service(); auto end=Clock::now();
            response.samples.push_back(std::max(0.0,elapsed_ns(scheduled,end)));
            service_time.samples.push_back(elapsed_ns(start,end));
            maximum_backlog=std::max(maximum_backlog,std::max(0.0,elapsed_ns(scheduled,start))/static_cast<double>(c.interval_ns));
            scheduled+=interval;
            if(c.duration>0 && std::chrono::duration<double>(end-begin).count()>=c.duration) break;
        }
        const auto cpu=thread_cpu_ns()-cpu_begin,wall=elapsed_ns(begin,Clock::now());
        response.metrics={{"thread_cpu_ns",cpu},{"measurement_wall_ns",wall},{"thread_cpu_fraction",cpu/wall},
            {"max_scheduled_backlog",maximum_backlog},{"main_cpu",static_cast<double>(main_cpu)},{"background_cpu",static_cast<double>(background_cpu)},
            {"background_threads",type=="idle"?0.0:1.0}};
        response.notes="Virtual constant-rate scheduled arrivals; response is planned-arrival to completion. No rebase/drop when late. Service time separate. Finite horizon, not a network server or independently enqueued traffic; all completed arrivals through the horizon retained. Background defaults to same CPU for deliberate contention; --background-cpu changes it.";
        service_time.notes="Service start-to-end includes preemption during work, excludes scheduled backlog. Pair with corresponding scheduled response; small service p99 can hide overload.";
        rows.push_back(std::move(response)); rows.push_back(std::move(service_time));
    }
    return rows;
}
}
