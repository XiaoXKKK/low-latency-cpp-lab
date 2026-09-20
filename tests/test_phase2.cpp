#include "lab/arena.hpp"
#include "lab/benchmark.hpp"
#include "lab/linux_memory.hpp"
#include "lab/locks.hpp"
#include <atomic>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <sys/mman.h>
#define CHECK(value) do { if(!(value)) throw std::runtime_error(#value); } while(false)
int main() {
    try {
        lab::Arena arena(512);
        auto* first=arena.allocate(64,64); CHECK(reinterpret_cast<std::uintptr_t>(first)%64==0);
        auto* second=arena.allocate(64,128); CHECK(first!=second && reinterpret_cast<std::uintptr_t>(second)%128==0);
        bool exhausted=false; try { arena.allocate(1024,8); } catch(const std::bad_alloc&) {exhausted=true;} CHECK(exhausted);
        arena.reset(); CHECK(arena.allocate(64,64)==first);
        lab::TicketLock lock; std::uint64_t counter=0; std::vector<std::thread> threads;
        for(int i=0;i<4;++i) threads.emplace_back([&]{for(int j=0;j<10000;++j) {std::lock_guard guard(lock);++counter;}});
        for(auto& thread:threads) thread.join();
        CHECK(counter==40000);
        // Two-direction release/acquire handshake: data is ordinary non-atomic memory.
        std::atomic<bool> ready{false}; std::uint64_t payload=0; bool correct=true;
        std::thread producer([&]{for(std::uint64_t i=1;i<=50000;++i) {
            while(ready.load(std::memory_order_acquire)) lab::pause_cpu();
            payload=i;ready.store(true,std::memory_order_release);
        }});
        for(std::uint64_t i=1;i<=50000;++i) {
            while(!ready.load(std::memory_order_acquire)) lab::pause_cpu();
            if(payload!=i) correct=false;
            ready.store(false,std::memory_order_release);
        }
        producer.join(); CHECK(correct);
        lab::MappedRegion mapping(4*lab::page_size(),false,2*1024*1024);
        CHECK(reinterpret_cast<std::uintptr_t>(mapping.data())%(2*1024*1024)==0);
        mapping.advise(MADV_NOHUGEPAGE);
        auto* bytes=static_cast<unsigned char*>(mapping.data());
        for(std::size_t i=0;i<mapping.size();i+=lab::page_size()) bytes[i]=42;
        auto info=lab::page_info(mapping); CHECK(info.exact); CHECK(info.resident_bytes==mapping.size()); CHECK(info.anon_huge_bytes==0);
        // Verify the Linux nodemask ABI when NUMA policy syscalls are permitted.
        try {
            lab::MappedRegion bound(lab::page_size());
            const int node=lab::cpu_node(lab::allowed_cpus().front());
            lab::bind_memory(bound,node); *static_cast<unsigned char*>(bound.data())=7;
            const auto nodes=lab::page_nodes(bound); CHECK(nodes.size()==1 && nodes.front()==node);
        } catch(const std::runtime_error& e) {
            const std::string reason=e.what();
            if(reason.find("Operation not permitted")==std::string::npos && reason.find("Permission denied")==std::string::npos && reason.find("unavailable")==std::string::npos)
                throw;
            std::cout<<"NUMA syscall validation NOT MEASURED: "<<reason<<'\n';
        }
        std::cout<<"arena alignment/exhaust/reset, ticket concurrency, repeated publication, mapping/smaps: PASS\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
