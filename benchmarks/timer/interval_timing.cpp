#include "lab/benchmark.hpp"
#include <cerrno>
#include <cstring>
#include <linux/perf_event.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#if defined(__x86_64__)
#include <cpuid.h>
#include <x86intrin.h>
#endif
namespace lab {
namespace {
void arithmetic(std::uint64_t& state,std::size_t steps) {
    for(std::size_t i=0;i<steps;++i) state=state*6364136223846793005ULL+1;
    do_not_optimize(state);
}
#if defined(__x86_64__)
struct Stamp { std::uint64_t ticks; unsigned aux; };
void serialize() { unsigned a,b,c,d; __cpuid(0,a,b,c,d); asm volatile("" : : "r"(a),"r"(b),"r"(c),"r"(d) : "memory"); }
Stamp stamp() {
    serialize(); unsigned aux=0; const auto value=__rdtscp(&aux); serialize(); return {value,aux};
}
#endif
class PerfGroup {
    int leader_=-1, instructions_=-1;
    std::uint64_t previous_enabled_=0, previous_running_=0;
public:
    PerfGroup() {
        perf_event_attr attr{}; attr.size=sizeof(attr); attr.type=PERF_TYPE_HARDWARE;
        attr.config=PERF_COUNT_HW_CPU_CYCLES; attr.disabled=1; attr.exclude_kernel=1; attr.exclude_hv=1;
        attr.read_format=PERF_FORMAT_GROUP|PERF_FORMAT_TOTAL_TIME_ENABLED|PERF_FORMAT_TOTAL_TIME_RUNNING;
        leader_=static_cast<int>(syscall(SYS_perf_event_open,&attr,0,-1,-1,PERF_FLAG_FD_CLOEXEC));
        if(leader_<0) throw std::runtime_error("perf_event_open cycles: "+std::string(std::strerror(errno)));
        attr.config=PERF_COUNT_HW_INSTRUCTIONS; attr.disabled=0;
        instructions_=static_cast<int>(syscall(SYS_perf_event_open,&attr,0,-1,leader_,PERF_FLAG_FD_CLOEXEC));
        if(instructions_<0) { auto reason=std::string(std::strerror(errno)); close(leader_); throw std::runtime_error("perf_event_open instructions: "+reason); }
    }
    ~PerfGroup() { if(instructions_>=0) close(instructions_); if(leader_>=0) close(leader_); }
    void begin() {
        if(ioctl(leader_,PERF_EVENT_IOC_RESET,PERF_IOC_FLAG_GROUP)<0 || ioctl(leader_,PERF_EVENT_IOC_ENABLE,PERF_IOC_FLAG_GROUP)<0)
            throw std::runtime_error("perf reset/enable failed");
    }
    struct Counts { std::uint64_t count, enabled, running, cycles, instructions; };
    Counts end() {
        if(ioctl(leader_,PERF_EVENT_IOC_DISABLE,PERF_IOC_FLAG_GROUP)<0) throw std::runtime_error("perf disable failed");
        Counts value{};
        if(read(leader_,&value,sizeof(value))!=sizeof(value) || value.count!=2 || value.running==0)
            throw std::runtime_error("perf group was not counted");
        const auto enabled=value.enabled, running=value.running;
        value.enabled-=previous_enabled_; value.running-=previous_running_;
        previous_enabled_=enabled; previous_running_=running;
        if(value.running==0) throw std::runtime_error("perf interval was not counted");
        return value;
    }
};
}
Results tsc_interval(const Config& c) {
    require_threads(c,1); Results rows;
    for(const std::string variant:{"empty","dependent_chain"}) {
        if(!selected(c,variant)) continue;
#if defined(__x86_64__)
        unsigned a=0,b=0,ecx=0,d=0;
        const bool rdtscp=__get_cpuid(0x80000001,&a,&b,&ecx,&d) && (d&(1U<<27));
        const bool invariant=__get_cpuid(0x80000007,&a,&b,&ecx,&d) && (d&(1U<<8));
        if(!rdtscp || !invariant) { rows.push_back(unavailable(c,"tsc_interval",variant,"RDTSCP/invariant TSC capability missing")); continue; }
        Result result; result.benchmark="tsc_interval"; result.variant=variant; result.mode="latency";
        result.unit="tsc_ticks/op"; result.sample_kind="batch_mean_serialized_tsc"; result.operations_per_sample=c.batch;
        std::uint64_t state=c.seed; std::size_t rejected=0;
        for(std::size_t i=0;i<c.warmup;++i) { auto x=stamp(); if(variant!="empty") arithmetic(state,c.batch); auto y=stamp(); do_not_optimize(x.ticks+y.ticks); }
        const auto begin=Clock::now();
        for(std::size_t i=0;i<c.iterations;++i) {
            const auto start=stamp(); if(variant!="empty") arithmetic(state,c.batch); else compiler_barrier(); const auto end=stamp();
            if(start.aux!=end.aux || end.ticks<start.ticks) ++rejected;
            else result.samples.push_back(static_cast<double>(end.ticks-start.ticks)/c.batch);
            if(c.duration>0 && std::chrono::duration<double>(Clock::now()-begin).count()>=c.duration) break;
        }
        result.metrics["rejected_aux_or_nonmonotonic"]=static_cast<double>(rejected);
        result.notes="CPUID/RDTSCP/CPUID at both ends. Includes serialization overhead; empty is per configured batch denominator, no subtraction. TSC ticks are not core cycles. AUX equality cannot detect migration away and back.";
        if(result.samples.empty()) { result.status="NOT MEASURED"; result.notes+=" All samples rejected."; }
        rows.push_back(std::move(result));
#else
        rows.push_back(unavailable(c,"tsc_interval",variant,"x86-64 required"));
#endif
    }
    return rows;
}
Results perf_interval(const Config& c) {
    require_threads(c,1); Results rows;
    for(const std::string variant:{"empty","dependent_chain"}) {
        if(!selected(c,variant)) continue;
        try {
            PerfGroup counters; std::uint64_t state=c.seed;
            // Warmup is outside the event-enabled interval.
            for(std::size_t i=0;i<c.warmup;++i) if(variant!="empty") arithmetic(state,c.batch);
            Config measured=c; measured.warmup=0;
            double cycles=0,instructions=0,enabled=0,running=0;
            auto r=measure(measured,"perf_interval",variant,c.batch,[&]{if(variant!="empty") arithmetic(state,c.batch); else compiler_barrier();},
                "throughput","batch_mean_perf_bracket",[&]{counters.begin();},[&]{
                    const auto values=counters.end(); cycles+=values.cycles; instructions+=values.instructions;
                    enabled+=values.enabled; running+=values.running;
                });
            // Group scheduling means both counters share the running intervals. Expose raw counts, never hide multiplexing.
            r.metrics={{"cycles_raw",cycles},{"instructions_raw",instructions},{"time_enabled_ns",enabled},{"time_running_ns",running}};
            if(cycles>0) r.metrics["ipc_raw"]=instructions/cycles;
            if(enabled==running) r.metrics["cycles_per_op"]=cycles/r.total_operations;
            r.notes="Calling thread only; excludes kernel/hypervisor and warmup. Interval includes two steady_clock reads/control overhead. cycles/op omitted if multiplexed; raw IPC requires caution. Empty control uses batch denominator.";
            rows.push_back(std::move(r));
        } catch(const std::runtime_error& e) { rows.push_back(unavailable(c,"perf_interval",variant,e.what())); }
    }
    return rows;
}
}
