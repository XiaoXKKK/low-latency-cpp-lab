#include "lab/benchmark.hpp"
#include "lab/structures.hpp"
#include <stdexcept>
namespace lab {
struct Message { std::uint64_t sequence=0; Clock::time_point sent{}; };
template<std::size_t Padding> void queue_run(const Config& c,const std::string& name,Results& rows) {
    SpscRing<Message,1024,Padding> queue;
    std::uint64_t errors=0;
    auto run=[&](bool timestamps,std::vector<double>& latency) {
        return measure_parallel(c,"spsc",name,c.batch,[&](unsigned t) {
            if(t==0) {
                for(std::size_t i=0;i<c.batch;++i) {
                    Message m; m.sequence=i;
                    if(timestamps) m.sent=Clock::now();
                    while(!queue.push(m)) pause_cpu();
                }
            } else {
                for(std::size_t i=0;i<c.batch;++i) {
                    Message m; while(!queue.pop(m)) pause_cpu();
                    if(timestamps) latency[i]=elapsed_ns(m.sent,Clock::now());
                    if(m.sequence!=i) ++errors;
                    do_not_optimize(m.sequence);
                }
            }
        });
    };
    std::vector<double> unused;
    auto throughput=run(false,unused);
    throughput.notes+=" messages/sec; ns/message is reciprocal throughput. Same algorithm, only cursor alignment changes.";
    rows.push_back(std::move(throughput));
    // Separate instrumented run, collect each message from each measured sample.
    std::vector<double> batch_latency(c.batch), samples;
    samples.reserve(c.iterations*c.batch);
    // Reuse worker infrastructure in one run; consumer commits samples after each batch.
    std::size_t sample_index=0;
    auto timing=measure_parallel(c,"spsc",name,c.batch,[&](unsigned t) {
        if(t==0) {
            for(std::size_t i=0;i<c.batch;++i) {
                Message m{i,Clock::now()}; while(!queue.push(m)) pause_cpu();
            }
        } else {
            for(std::size_t i=0;i<c.batch;++i) {
                Message m; while(!queue.pop(m)) pause_cpu();
                batch_latency[i]=elapsed_ns(m.sent,Clock::now());
                if(m.sequence!=i) ++errors;
            }
            if(sample_index++>=c.warmup) samples.insert(samples.end(),batch_latency.begin(),batch_latency.end());
        }
    });
    (void)timing;
    if(errors) throw std::runtime_error("SPSC FIFO violation");
    Result latency; latency.benchmark="spsc"; latency.variant=name+"_latency"; latency.mode="latency";
    latency.sample_kind="individual_message_instrumented"; latency.threads=2; latency.samples=std::move(samples);
    latency.notes="Separate timestamped pass: enqueue-attempt to dequeue completion, includes queueing/backpressure and clock overhead. Closed-loop bounded producer; not an open-loop service latency.";
    rows.push_back(std::move(latency));
}
Results spsc(const Config& c) {
    require_threads(c,2);
    if(c.iterations*c.batch>10000000) throw std::invalid_argument("SPSC latency sample memory cap: iterations*batch <= 10M");
    Results rows;
    if(selected(c,"naive")) queue_run<alignof(std::atomic<std::size_t>)>(c,"naive",rows);
    // Cache line runtime validation prevents claiming isolation on wider lines.
    if(selected(c,"padded")) {
        if(cache_line_size()>128 || 128%cache_line_size()!=0) throw std::runtime_error("128-byte queue padding incompatible with detected cache line");
        queue_run<128>(c,"padded",rows);
    }
    return rows;
}
}
