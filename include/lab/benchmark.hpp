#pragma once
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace lab {
using Clock = std::chrono::steady_clock;
struct Config {
    std::string benchmark = "timer", variant, format = "table";
    std::size_t iterations = 100, warmup = 10, batch = 4096, size = 32768;
    unsigned threads = 1;
    double duration = 0;
    std::uint64_t seed = 42;
    std::vector<int> cpus;
    std::size_t critical = 0, locks = 1;
    std::size_t stride = 1, distance = 16, interval_ns = 100000, read_percent = 90;
    int memory_node = -1, touch_cpu = -1, background_cpu = -1;
};
struct Stats { double mean{}, median{}, min{}, max{}, p50{}, p90{}, p95{}, p99{}, p999{}, stddev{}; };
Stats statistics(std::vector<double> values);
Config parse(int argc, char** argv);
std::vector<int> allowed_cpus();
std::size_t cache_line_size();
void pin_current(int cpu);
class AffinityGuard {
    std::vector<int> saved_;
public:
    AffinityGuard();
    ~AffinityGuard();
};
template<class T> inline void do_not_optimize(const T& value) {
    asm volatile("" : : "g"(value) : "memory");
}
inline void compiler_barrier() { asm volatile("" : : : "memory"); }
inline void pause_cpu() {
#if defined(__x86_64__) || defined(__i386__)
    asm volatile("pause" ::: "memory");
#else
    compiler_barrier();
#endif
}
inline double elapsed_ns(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::nano>(b-a).count();
}
struct Result {
    std::string benchmark, variant, mode = "throughput", sample_kind = "batch_mean", unit = "ns/op";
    std::string notes, status = "MEASURED";
    std::map<std::string, double> metrics;
    unsigned threads = 1;
    std::size_t operations_per_sample = 1;
    std::vector<double> samples;
    double total_ns = 0;
    double total_operations = 0;
};
using Results = std::vector<Result>;
bool selected(const Config& c, const std::string& variant);
void require_threads(const Config& c, unsigned n);
Result measure(const Config& c, std::string name, std::string variant,
               std::size_t operations, const std::function<void()>& body,
               std::string mode = "throughput", std::string sample_kind = "batch_mean",
               const std::function<void()>& before = {}, const std::function<void()>& after = {});
Result measure_parallel(const Config& c, std::string name, std::string variant,
                        std::size_t operations, const std::function<void(unsigned)>& body);
Result unavailable(const Config&, std::string name, std::string variant, std::string reason);
void output(const Config& c, const Results& results);
Results timer(const Config&); Results memory_access(const Config&); Results false_sharing(const Config&);
Results branch(const Config&); Results allocation(const Config&); Results locks(const Config&);
Results spsc(const Config&); Results affinity(const Config&);
Results tsc_interval(const Config&); Results perf_interval(const Config&);
Results cache_patterns(const Config&); Results allocator_batch(const Config&);
Results allocation_handoff(const Config&); Results page_behavior(const Config&);
Results atomic_order(const Config&); Results rw_locks(const Config&);
Results wait_strategy(const Config&); Results arrival_latency(const Config&);
Results numa_access(const Config&);
} // namespace lab
