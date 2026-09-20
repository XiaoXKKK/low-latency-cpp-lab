#pragma once
#include "lab/benchmark.hpp"
namespace lab {
class TicketLock {
    alignas(128) std::atomic<std::uint64_t> next_{0};
    alignas(128) std::atomic<std::uint64_t> serving_{0};
public:
    void lock() noexcept {
        const auto ticket=next_.fetch_add(1,std::memory_order_relaxed);
        while(serving_.load(std::memory_order_acquire)!=ticket) pause_cpu();
    }
    void unlock() noexcept { serving_.fetch_add(1,std::memory_order_release); }
};
}
