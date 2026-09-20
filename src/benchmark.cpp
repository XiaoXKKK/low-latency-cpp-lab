#include "lab/benchmark.hpp"
#include <algorithm>
#include <barrier>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <pthread.h>
#include <sched.h>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unistd.h>

namespace lab {
Stats statistics(std::vector<double> v) {
    if (v.empty()) throw std::invalid_argument("empty samples");
    for (auto x : v) if (!std::isfinite(x) || x < 0) throw std::invalid_argument("invalid sample");
    std::sort(v.begin(), v.end());
    const auto percentile = [&](double p) {
        double pos = p * static_cast<double>(v.size()-1);
        auto lo = static_cast<std::size_t>(pos), hi = std::min(lo+1, v.size()-1);
        return v[lo] + (v[hi]-v[lo])*(pos-static_cast<double>(lo));
    };
    Stats s;
    s.mean = std::accumulate(v.begin(), v.end(), 0.0)/static_cast<double>(v.size());
    s.min=v.front(); s.max=v.back(); s.median=s.p50=percentile(.5);
    s.p90=percentile(.9); s.p95=percentile(.95); s.p99=percentile(.99); s.p999=percentile(.999);
    for (double x : v) s.stddev += (x-s.mean)*(x-s.mean);
    s.stddev=std::sqrt(s.stddev/static_cast<double>(v.size())); // population SD
    return s;
}
static std::size_t positive_integer(const std::string& v, bool zero=false) {
    if (v.empty() || v.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("expected nonnegative integer: " + v);
    std::size_t pos=0;
    auto n=std::stoull(v, &pos);
    if (pos != v.size() || (!zero && n==0) || n>std::numeric_limits<std::size_t>::max())
        throw std::invalid_argument("integer out of range: " + v);
    return static_cast<std::size_t>(n);
}
Config parse(int argc, char** argv) {
    Config c;
    bool explicit_size=false, explicit_batch=false;
    for (int i=1; i<argc; ++i) {
        std::string key=argv[i];
        if (i+1==argc) throw std::invalid_argument("missing value for " + key);
        std::string v=argv[++i];
        if (key=="--benchmark") c.benchmark=v;
        else if (key=="--variant") c.variant=v;
        else if (key=="--format") c.format=v;
        else if (key=="--iterations") c.iterations=positive_integer(v);
        else if (key=="--warmup") c.warmup=positive_integer(v,true);
        else if (key=="--batch") { c.batch=positive_integer(v); explicit_batch=true; }
        else if (key=="--size") { c.size=positive_integer(v); explicit_size=true; }
        else if (key=="--timeout-ms") {
            auto n=positive_integer(v); if(n>60000) throw std::invalid_argument("network timeout <=60000 ms required");
            c.timeout_ms=static_cast<unsigned>(n);
        }
        else if (key=="--seed") c.seed=positive_integer(v,true);
        else if (key=="--critical") c.critical=positive_integer(v,true);
        else if (key=="--locks") c.locks=positive_integer(v);
        else if (key=="--stride") c.stride=positive_integer(v);
        else if (key=="--distance") c.distance=positive_integer(v,true);
        else if (key=="--interval-ns") c.interval_ns=positive_integer(v);
        else if (key=="--read-percent") c.read_percent=positive_integer(v,true);
        else if (key=="--memory-node" || key=="--touch-cpu" || key=="--background-cpu") {
            int n=-1;
            if(v!="-1") { auto parsed=positive_integer(v,true); if(parsed>=CPU_SETSIZE) throw std::invalid_argument("node/CPU out of range"); n=static_cast<int>(parsed); }
            if(key=="--memory-node") c.memory_node=n;
            else if(key=="--touch-cpu") c.touch_cpu=n;
            else c.background_cpu=n;
        }
        else if (key=="--threads") {
            auto n=positive_integer(v); if(n>1024) throw std::invalid_argument("threads > 1024");
            c.threads=static_cast<unsigned>(n);
        } else if (key=="--duration") {
            std::size_t pos=0; c.duration=std::stod(v,&pos);
            if(pos!=v.size() || !std::isfinite(c.duration) || c.duration<0 || c.duration>3600)
                throw std::invalid_argument("duration must be finite, 0..3600 seconds");
        } else if (key=="--cpu") {
            if(v=="-1") continue;
            std::stringstream ss(v); std::string item;
            if(v.empty() || v.back()==',') throw std::invalid_argument("empty cpu");
            while(std::getline(ss,item,',')) {
                auto cpu=positive_integer(item,true);
                if(cpu>=CPU_SETSIZE) throw std::invalid_argument("CPU beyond CPU_SETSIZE");
                c.cpus.push_back(static_cast<int>(cpu));
            }
        } else throw std::invalid_argument("unknown option: " + key);
    }
    if(c.benchmark=="network_rtt" || c.benchmark=="network_io") {
        if(!explicit_size) c.size=64;
        if(!explicit_batch) c.batch=16;
    }
    if(c.format!="table" && c.format!="json" && c.format!="csv") throw std::invalid_argument("format: table/json/csv");
    if(c.iterations>10000000 || c.warmup>1000000 || c.batch>100000000 || c.size>1073741824 || c.critical>1000000 || c.locks>100000)
        throw std::invalid_argument("workload exceeds safety bound");
    if(c.stride>1048576 || c.distance>1048576 || c.interval_ns>1000000000 || c.read_percent>100)
        throw std::invalid_argument("phase2 parameter out of range");
    const auto allowed=allowed_cpus();
    for(int cpu:{c.touch_cpu,c.background_cpu}) if(cpu>=0 && std::find(allowed.begin(),allowed.end(),cpu)==allowed.end())
        throw std::invalid_argument("auxiliary CPU not in allowed mask");
    for(int cpu:c.cpus) if(std::find(allowed.begin(),allowed.end(),cpu)==allowed.end())
        throw std::invalid_argument("CPU not in process allowed mask: " + std::to_string(cpu));
    return c;
}
std::vector<int> allowed_cpus() {
    cpu_set_t mask; CPU_ZERO(&mask);
    if(sched_getaffinity(0,sizeof(mask),&mask)!=0) throw std::runtime_error(std::strerror(errno));
    std::vector<int> cpus;
    for(int i=0;i<CPU_SETSIZE;++i) if(CPU_ISSET(i,&mask)) cpus.push_back(i);
    return cpus;
}
void pin_current(int cpu) {
    cpu_set_t mask; CPU_ZERO(&mask); CPU_SET(cpu,&mask);
    int err=pthread_setaffinity_np(pthread_self(),sizeof(mask),&mask);
    if(err) throw std::runtime_error("affinity: " + std::string(std::strerror(err)));
}
AffinityGuard::AffinityGuard():saved_(allowed_cpus()) {}
AffinityGuard::~AffinityGuard() {
    cpu_set_t mask; CPU_ZERO(&mask); for(int cpu:saved_) CPU_SET(cpu,&mask);
    if(pthread_setaffinity_np(pthread_self(),sizeof(mask),&mask)!=0) std::terminate();
}
std::size_t cache_line_size() {
    long n=sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if(n>0) return static_cast<std::size_t>(n);
    std::ifstream f("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size");
    std::size_t value=0; f>>value;
    if(value==0) throw std::runtime_error("Cannot detect cache line size");
    return value;
}
bool selected(const Config& c,const std::string& v) { return c.variant.empty() || c.variant==v; }
void require_threads(const Config& c,unsigned n) {
    if(c.threads!=n) throw std::invalid_argument(c.benchmark+" requires --threads "+std::to_string(n));
}
Result measure(const Config& c,std::string name,std::string variant,std::size_t ops,
               const std::function<void()>& body,std::string mode,std::string kind,
               const std::function<void()>& before,const std::function<void()>& after) {
    Result r; r.benchmark=std::move(name); r.variant=std::move(variant); r.mode=std::move(mode);
    r.sample_kind=std::move(kind); r.operations_per_sample=ops; r.threads=c.threads;
    r.samples.reserve(c.iterations);
    for(std::size_t i=0;i<c.warmup;++i) { if(before) before(); body(); if(after) after(); }
    const auto begin=Clock::now();
    for(std::size_t i=0;i<c.iterations;++i) {
        if(before) before();
        compiler_barrier(); auto a=Clock::now(); body(); compiler_barrier(); auto b=Clock::now();
        if(after) after();
        double ns=elapsed_ns(a,b); r.samples.push_back(ns/static_cast<double>(ops));
        r.total_ns+=ns; r.total_operations+=static_cast<double>(ops);
        if(c.duration>0 && std::chrono::duration<double>(b-begin).count()>=c.duration) break;
    }
    return r;
}
Result measure_parallel(const Config& c,std::string name,std::string variant,std::size_t ops,
                        const std::function<void(unsigned)>& body) {
    // Persistent workers: construction/affinity are outside all samples.
    std::barrier ready(static_cast<std::ptrdiff_t>(c.threads+1));
    std::barrier go(static_cast<std::ptrdiff_t>(c.threads+1));
    std::barrier done(static_cast<std::ptrdiff_t>(c.threads+1));
    std::atomic<bool> stop{false};
    std::vector<std::exception_ptr> errors(c.threads);
    std::vector<std::thread> workers;
    // Launch gate also makes partial thread creation failure safe.
    std::atomic<int> launch{0};
    try {
        for(unsigned t=0;t<c.threads;++t) workers.emplace_back([&,t] {
            while(launch.load(std::memory_order_acquire)==0) std::this_thread::yield();
            if(launch.load(std::memory_order_acquire)<0) return;
            try { if(!c.cpus.empty()) pin_current(c.cpus[t%c.cpus.size()]); } catch(...) { errors[t]=std::current_exception(); }
            ready.arrive_and_wait();
            for(;;) {
                go.arrive_and_wait(); if(stop.load(std::memory_order_relaxed)) break;
                try { if(!errors[t]) body(t); } catch(...) { errors[t]=std::current_exception(); }
                done.arrive_and_wait();
            }
        });
    } catch(...) {
        launch.store(-1,std::memory_order_release); for(auto& w:workers) w.join(); throw;
    }
    launch.store(1,std::memory_order_release); ready.arrive_and_wait();
    auto shutdown=[&] { stop.store(true,std::memory_order_relaxed); go.arrive_and_wait(); for(auto& w:workers) w.join(); };
    for(const auto& e:errors) if(e) { shutdown(); std::rethrow_exception(e); }
    Result r;
    try {
        r=measure(c,std::move(name),std::move(variant),ops,[&] { go.arrive_and_wait(); done.arrive_and_wait(); });
    } catch(...) { shutdown(); throw; }
    shutdown();
    for(const auto& e:errors) if(e) std::rethrow_exception(e);
    r.notes="Persistent workers; timed region includes start/end barriers and scheduler wakeups; aggregate ns/op is not operation latency.";
    return r;
}
} // namespace lab
