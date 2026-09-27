#pragma once
#include <cerrno>
#include <array>
#include <cstdint>
#include <cstring>
#include <linux/perf_event.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace lab::book {
// Optional calling-thread user-space PMU. Each complete replay gets an interval;
// initialization, oracle generation, warmup, reset and validation are disabled.
class ReplayCounters {
    int cycles_ = -1;
    std::array<int, 4> members_{{-1, -1, -1, -1}};
    std::uint64_t previous_enabled_ = 0, previous_running_ = 0;
public:
    struct Counts { std::uint64_t count, enabled, running, cycles, instructions, branches, branch_misses, cache_misses; };
    ReplayCounters() {
        perf_event_attr attr{};
        attr.size = sizeof(attr); attr.type = PERF_TYPE_HARDWARE; attr.config = PERF_COUNT_HW_CPU_CYCLES;
        attr.disabled = 1; attr.exclude_kernel = 1; attr.exclude_hv = 1;
        attr.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
        cycles_ = static_cast<int>(syscall(SYS_perf_event_open, &attr, 0, -1, -1, PERF_FLAG_FD_CLOEXEC));
        if(cycles_ < 0) throw std::runtime_error(std::string("cycles: ") + std::strerror(errno));
        constexpr std::array events{PERF_COUNT_HW_INSTRUCTIONS, PERF_COUNT_HW_BRANCH_INSTRUCTIONS,
                                    PERF_COUNT_HW_BRANCH_MISSES, PERF_COUNT_HW_CACHE_MISSES};
        for(std::size_t i = 0; i < events.size(); ++i) {
            attr.config = events[i]; attr.disabled = 0;
            members_[i] = static_cast<int>(syscall(SYS_perf_event_open, &attr, 0, -1, cycles_, PERF_FLAG_FD_CLOEXEC));
            if(members_[i] < 0) {
                const auto reason = std::string("PMU member: ") + std::strerror(errno);
                for(int fd : members_) if(fd >= 0) close(fd);
                close(cycles_); throw std::runtime_error(reason);
            }
        }
    }
    ReplayCounters(const ReplayCounters&) = delete;
    ReplayCounters& operator=(const ReplayCounters&) = delete;
    ~ReplayCounters() { for(int fd : members_) if(fd >= 0) close(fd); if(cycles_ >= 0) close(cycles_); }
    void begin() {
        if(ioctl(cycles_, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) < 0 ||
           ioctl(cycles_, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) < 0) throw std::runtime_error("PMU enable failed");
    }
    Counts end() {
        if(ioctl(cycles_, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP) < 0) throw std::runtime_error("PMU disable failed");
        Counts result{};
        if(read(cycles_, &result, sizeof(result)) != sizeof(result) || result.count != 5)
            throw std::runtime_error("PMU read failed");
        const auto enabled = result.enabled, running = result.running;
        result.enabled -= previous_enabled_; result.running -= previous_running_;
        previous_enabled_ = enabled; previous_running_ = running;
        if(!result.running) throw std::runtime_error("PMU interval not counted");
        return result;
    }
};
} // namespace lab::book
