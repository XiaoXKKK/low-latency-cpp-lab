#include "lab/benchmark.hpp"
#include "lab/structures.hpp"
#include <cmath>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#define CHECK(x) do { if(!(x)) throw std::runtime_error(#x); } while(false)
template<std::size_t Padding> void queue_test() {
    lab::SpscRing<std::uint64_t,8,Padding> q;
    std::uint64_t x=99; CHECK(!q.pop(x));
    for(std::uint64_t i=0;i<7;++i) CHECK(q.push(i));
    CHECK(!q.push(8));
    for(std::uint64_t i=0;i<7;++i) { CHECK(q.pop(x)); CHECK(x==i); }
    CHECK(!q.pop(x));
    constexpr std::uint64_t n=200000; std::atomic<bool> good{true};
    std::thread producer([&] { for(std::uint64_t i=0;i<n;++i) while(!q.push(i)) lab::pause_cpu(); });
    std::thread consumer([&] { for(std::uint64_t i=0;i<n;++i) { std::uint64_t value=0; while(!q.pop(value)) lab::pause_cpu(); if(value!=i) good=false; } });
    producer.join(); consumer.join(); CHECK(good); CHECK(!q.pop(x));
}
int main() {
    try {
        auto s=lab::statistics({1,2,3,4,5});
        CHECK(s.mean==3 && s.median==3 && s.min==1 && s.max==5);
        CHECK(std::abs(s.p99-4.96)<1e-9); CHECK(std::abs(s.stddev-std::sqrt(2.0))<1e-9);
        CHECK(lab::statistics({7}).p999==7);
        bool threw=false; try { lab::statistics({}); } catch(const std::invalid_argument&) { threw=true; } CHECK(threw);
        queue_test<alignof(std::atomic<std::size_t>)>(); queue_test<128>();
        lab::FixedPool pool(2); auto* a=pool.allocate(); auto* b=pool.allocate(); CHECK(a!=b);
        threw=false; try { pool.allocate(); } catch(const std::bad_alloc&) { threw=true; } CHECK(threw);
        pool.deallocate(a); CHECK(pool.allocate()==a); pool.deallocate(a); pool.deallocate(b);
        lab::SpinLock lock; std::uint64_t count=0;
        std::vector<std::thread> workers;
        for(int t=0;t<4;++t) workers.emplace_back([&]{ for(int i=0;i<10000;++i) { std::lock_guard guard(lock); ++count; } });
        for(auto& t:workers) t.join();
        CHECK(count==40000);
        // Release/acquire publishes ordinary payload without a data race.
        int payload=0; std::atomic<bool> ready{false};
        std::thread producer([&]{payload=42;ready.store(true,std::memory_order_release);});
        while(!ready.load(std::memory_order_acquire)) lab::pause_cpu();
        CHECK(payload==42); producer.join();
        auto before=lab::allowed_cpus(); { lab::AffinityGuard restore; lab::pin_current(before.front()); CHECK(lab::allowed_cpus().size()==1); }
        CHECK(lab::allowed_cpus()==before);
        std::cout<<"statistics, FIFO wrap/full/empty/concurrent, pool, spinlock, publication, affinity: PASS\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
