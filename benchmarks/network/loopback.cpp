#include "lab/benchmark.hpp"
#include "lab/network.hpp"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <exception>
#include <future>
#include <span>
#include <sys/socket.h>
#include <thread>
#include <time.h>

namespace lab {
namespace {
double process_cpu_ns() {
    timespec time{};
    if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&time)!=0) throw std::runtime_error("process CPU clock");
    return time.tv_sec*1e9+time.tv_nsec;
}
Result run(const Config& c,const std::string& variant,bool udp,bool batched,bool coalesce,net::Mode mode,bool nodelay) {
    const auto messages=batched?c.batch:1;
    if(c.size<8 || c.size>65507 || messages>256 || messages*c.size>4*1024*1024)
        throw std::invalid_argument("network: size 8..65507, batch <=256, window <=4 MiB required");
    auto sockets=net::loopback_pair(udp,nodelay);
    net::Channel client(sockets.client.get(),mode,c.timeout_ms);
    net::Channel server(sockets.server.get(),mode,c.timeout_ms);
    std::vector<std::byte> outgoing(messages*c.size),incoming(messages*c.size),temporary(c.size),echo(udp?c.size:messages*c.size);
    for(std::size_t i=0;i<outgoing.size();++i) outgoing[i]=static_cast<std::byte>((i*131+c.seed)%256);
    std::vector<bool> seen(messages);
    std::atomic<bool> stop{false};
    std::exception_ptr server_error;
    std::promise<void> ready;
    auto started=ready.get_future();
    // Client owns request/response buffers; worker owns echo buffer and server
    // Channel. Only stop is shared during execution. join publishes worker error.
    std::thread worker([&] {
        bool announced=false;
        try {
            if(!c.cpus.empty()) pin_current(c.cpus[1%c.cpus.size()]);
            ready.set_value(); announced=true;
            while(!stop.load(std::memory_order_acquire)) {
                if(udp) {
                    std::size_t count=0;
                    try { count=server.receive_datagram(echo,net::deadline(c.timeout_ms)); }
                    catch(const net::Timeout&) { continue; }
                    if(stop.load(std::memory_order_acquire)) break;
                    server.send_datagram(std::span(echo).first(count),net::deadline(c.timeout_ms));
                } else {
                    server.receive_all(echo,net::deadline(c.timeout_ms));
                    if(stop.load(std::memory_order_acquire)) break;
                    server.send_all(echo,net::deadline(c.timeout_ms));
                }
            }
        } catch(...) {
            if(!announced) ready.set_exception(std::current_exception());
            if(!stop.load(std::memory_order_acquire)) server_error=std::current_exception();
            shutdown(sockets.server.get(),SHUT_RDWR);
        }
    });
    auto finish=[&] {
        stop.store(true,std::memory_order_release);
        shutdown(sockets.client.get(),SHUT_RDWR); shutdown(sockets.server.get(),SHUT_RDWR);
        worker.join();
    };
    Result result; result.benchmark=c.benchmark; result.variant=variant; result.threads=2;
    result.mode=batched?"throughput":"latency";
    result.unit=batched?"ns/sent_message":"ns/roundtrip";
    result.sample_kind=batched?"window_mean_roundtrip":"successful_closed_loop_rtt";
    result.operations_per_sample=messages; result.samples.reserve(c.iterations);
    std::uint64_t sequence=0,attempted=0,completed=0,unanswered=0,stale=0,duplicates=0,reordered=0;
    double measured_wall=0,cpu=0;
    try {
        started.get();
        if(!c.cpus.empty()) pin_current(c.cpus.front());
        auto sample=[&](bool measured) {
            const auto base=sequence; sequence+=messages;
            for(std::size_t m=0;m<messages;++m) {
                const auto id=base+m;
                std::memcpy(outgoing.data()+m*c.size,&id,sizeof(id));
            }
            std::fill(seen.begin(),seen.end(),false);
            std::size_t received=0;
            const auto begin=Clock::now(); const auto end=net::deadline(c.timeout_ms);
            if(udp) {
                // A send failure is a run failure; a missing echo is recorded as
                // unanswered within the deadline, not silently discarded.
                for(std::size_t m=0;m<messages;++m)
                    client.send_datagram(std::span(outgoing).subspan(m*c.size,c.size),end);
                while(received<messages) {
                    std::size_t count=0;
                    try { count=client.receive_datagram(temporary,end); }
                    catch(const net::Timeout&) { break; }
                    if(count!=c.size) throw std::runtime_error("UDP echo size mismatch");
                    std::uint64_t id=0; std::memcpy(&id,temporary.data(),sizeof(id));
                    if(id<base || id>=base+messages) { if(measured) ++stale; continue; }
                    const auto offset=static_cast<std::size_t>(id-base);
                    if(seen[offset]) { if(measured) ++duplicates; continue; }
                    if(offset!=received && measured) ++reordered;
                    std::copy(temporary.begin(),temporary.end(),incoming.begin()+offset*c.size);
                    seen[offset]=true; ++received;
                }
            } else {
                if(coalesce) client.send_all(outgoing,end);
                else for(std::size_t m=0;m<messages;++m)
                    client.send_all(std::span(outgoing).subspan(m*c.size,c.size),end);
                client.receive_all(incoming,end); received=messages;
            }
            const double ns=elapsed_ns(begin,Clock::now());
            // Byte-for-byte payload/sequence validation is outside the RTT bracket.
            for(std::size_t m=0;m<messages;++m)
                if((!udp || seen[m]) && !std::equal(outgoing.begin()+m*c.size,outgoing.begin()+(m+1)*c.size,incoming.begin()+m*c.size))
                    throw std::runtime_error("echo payload mismatch");
            if(measured) {
                attempted+=messages; completed+=received; unanswered+=messages-received;
                if(batched || received) result.samples.push_back(ns/messages);
                result.total_ns+=ns; result.total_operations+=received;
            }
        };
        for(std::size_t i=0;i<c.warmup;++i) sample(false);
        client.counters={};
        const auto begin=Clock::now(); const auto cpu_begin=process_cpu_ns();
        for(std::size_t i=0;i<c.iterations;++i) {
            sample(true);
            if(c.duration>0 && elapsed_ns(begin,Clock::now())>=c.duration*1e9) break;
        }
        cpu=process_cpu_ns()-cpu_begin; measured_wall=elapsed_ns(begin,Clock::now());
        finish();
    } catch(...) { finish(); throw; }
    if(server_error) std::rethrow_exception(server_error);
    result.metrics={{"attempted_messages",static_cast<double>(attempted)}, {"completed_messages",static_cast<double>(completed)},
        {"unanswered_messages",static_cast<double>(unanswered)}, {"unanswered_fraction",attempted?static_cast<double>(unanswered)/attempted:0},
        {"stale_replies",static_cast<double>(stale)}, {"duplicate_replies",static_cast<double>(duplicates)},
        {"out_of_order_replies",static_cast<double>(reordered)}, {"process_cpu_ns",cpu},
        {"measurement_wall_ns",measured_wall}, {"process_cpu_cores",measured_wall>0?cpu/measured_wall:0},
        {"client_send_calls",static_cast<double>(client.counters.sends)}, {"client_recv_calls",static_cast<double>(client.counters.receives)},
        {"client_short_io",static_cast<double>(client.counters.partial)}, {"client_eagain",static_cast<double>(client.counters.eagain)},
        {"client_wait_calls",static_cast<double>(client.counters.waits)}, {"payload_bytes",static_cast<double>(c.size)},
        {"window_messages",static_cast<double>(messages)}, {"tcp_nodelay",udp?0:static_cast<double>(nodelay)}};
    if(result.total_ns>0) result.metrics["completed_payload_bytes_per_sec"]=completed*c.size*1e9/result.total_ns;
    result.notes="IPv4 loopback echo, one persistent connection and two threads. Setup/affinity/warmup/byte validation outside sample bracket. CPU cores = process CPU / measured-loop wall (includes validation), can exceed 1. RTT includes both directions and scheduling; no NIC/wire inference. ";
    result.notes+=batched?"Batch sample = window completion / sent messages, not individual latency; throughput counts completed echoes (one-way payload bytes). ":"Closed-loop successful RTT only; timeouts excluded from RTT percentiles and counted separately. --batch unused. ";
    if(udp) result.notes+="UDP sequence parsing/copies inside bracket; unanswered means no valid echo by deadline, not proven kernel/NIC loss. No retransmission. ";
    else result.notes+="TCP peer reads entire window then echoes it; unbatched sends each message, batched sends contiguous window. ";
    if(mode==net::Mode::busy) result.notes+="Userspace recv/send retry with PAUSE, not SO_BUSY_POLL. ";
    if(mode==net::Mode::blocking) result.notes+="Blocking socket timeouts track remaining operation deadline rounded up to ms; kernel/scheduler can delay wakeup. ";
    if(result.samples.empty()) { result.status="NOT MEASURED"; result.notes+="No successful RTT samples."; }
    return result;
}
}
Results network_rtt(const Config& c) {
    require_threads(c,2); Results rows;
    for(const std::string variant:{"tcp_default","tcp_nodelay","tcp_unbatched","tcp_batched","udp_rtt","udp_batch"}) {
        if(!selected(c,variant)) continue;
        const bool udp=variant.starts_with("udp");
        rows.push_back(run(c,variant,udp,variant=="tcp_unbatched" || variant=="tcp_batched" || variant=="udp_batch",
                           variant=="tcp_batched",net::Mode::blocking,variant!="tcp_default"));
    }
    return rows;
}
Results network_io(const Config& c) {
    require_threads(c,2); Results rows;
    for(const auto& [variant,mode]:{std::pair{"blocking",net::Mode::blocking},{"epoll_lt",net::Mode::epoll_lt},
                                  {"epoll_et",net::Mode::epoll_et},{"busy_poll",net::Mode::busy}})
        if(selected(c,variant)) rows.push_back(run(c,variant,false,false,false,mode,true));
    return rows;
}
}
