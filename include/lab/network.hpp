#pragma once
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>

namespace lab::net {
class Fd {
    int value_=-1;
public:
    explicit Fd(int value=-1):value_(value) {}
    ~Fd();
    Fd(const Fd&)=delete;
    Fd& operator=(const Fd&)=delete;
    Fd(Fd&& other) noexcept:value_(std::exchange(other.value_,-1)) {}
    Fd& operator=(Fd&& other) noexcept;
    int get() const { return value_; }
};
enum class Mode { blocking, epoll_lt, epoll_et, busy };
class Timeout:public std::runtime_error { public: Timeout():std::runtime_error("network operation timed out") {} };
class PeerClosed:public std::runtime_error { public: PeerClosed():std::runtime_error("peer closed during frame") {} };
using Deadline=std::chrono::steady_clock::time_point;
Deadline deadline(unsigned timeout_ms);
struct Pair { Fd client, server; };
Pair loopback_pair(bool datagram, bool nodelay);
struct Counters { std::uint64_t sends=0, receives=0, partial=0, eagain=0, waits=0; };
// One thread owns a Channel and its buffers. The descriptor owner outlives it.
// Exact stream transfers preserve progress across short I/O and EAGAIN. ET only
// waits after EAGAIN; a later operation first tries I/O, retaining readiness.
class Channel {
    int fd_;
    Mode mode_;
    Fd epoll_;
    std::uint32_t interest_=0;
    unsigned blocking_timeout_ms_=0;
    void wait(bool writing, Deadline end);
    void check(Deadline end);
public:
    Counters counters;
    Channel(int fd, Mode mode, unsigned timeout_ms);
    void send_all(std::span<const std::byte> bytes, Deadline end);
    void receive_all(std::span<std::byte> bytes, Deadline end);
    void send_datagram(std::span<const std::byte> bytes, Deadline end);
    std::size_t receive_datagram(std::span<std::byte> bytes, Deadline end);
};
}
