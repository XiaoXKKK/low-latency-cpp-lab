#pragma once
#include <cerrno>
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
    int cycles_ = -1, instructions_ = -1;
    std::uint64_t previous_enabled_ = 0, previous_running_ = 0;
public:
    struct Counts { std::uint64_t count, enabled, running, cycles, instructions; };
    ReplayCounters() {
        perf_event_attr attr{};
        attr.size = sizeof(attr); attr.type = PERF_TYPE_HARDWARE; attr.config = PERF_COUNT_HW_CPU_CYCLES;
        attr.disabled = 1; attr.exclude_kernel = 1; attr.exclude_hv = 1;
        attr.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
        cycles_ = static_cast<int>(syscall(SYS_perf_event_open, &attr, 0, -1, -1, PERF_FLAG_FD_CLOEXEC));
        if(cycles_ < 0) throw std::runtime_error(std::string("cycles: ") + std::strerror(errno));
        attr.config = PERF_COUNT_HW_INSTRUCTIONS; attr.disabled = 0;
        instructions_ = static_cast<int>(syscall(SYS_perf_event_open, &attr, 0, -1, cycles_, PERF_FLAG_FD_CLOEXEC));
        if(instructions_ < 0) {
            const auto reason = std::string("instructions: ") + std::strerror(errno);
            close(cycles_); throw std::runtime_error(reason);
        }
    }
    ReplayCounters(const ReplayCounters&) = delete;
    ReplayCounters& operator=(const ReplayCounters&) = delete;
    ~ReplayCounters() { if(instructions_ >= 0) close(instructions_); if(cycles_ >= 0) close(cycles_); }
    void begin() {
        if(ioctl(cycles_, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) < 0 ||
           ioctl(cycles_, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) < 0) throw std::runtime_error("PMU enable failed");
    }
    Counts end() {
        if(ioctl(cycles_, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP) < 0) throw std::runtime_error("PMU disable failed");
        Counts result{};
        if(read(cycles_, &result, sizeof(result)) != sizeof(result) || result.count != 2)
            throw std::runtime_error("PMU read failed");
        const auto enabled = result.enabled, running = result.running;
        result.enabled -= previous_enabled_; result.running -= previous_running_;
        previous_enabled_ = enabled; previous_running_ = running;
        if(!result.running) throw std::runtime_error("PMU interval not counted");
        return result;
    }
};
} // namespace lab::book
