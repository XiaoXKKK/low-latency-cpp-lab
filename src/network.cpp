#include "lab/network.hpp"
#include "lab/benchmark.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace lab::net {
namespace {
[[noreturn]] void fail(const char* operation) { throw std::runtime_error(std::string(operation)+": "+std::strerror(errno)); }
bool again() { return errno==EAGAIN || errno==EWOULDBLOCK; }
Fd socket_fd(bool datagram) {
    int fd=socket(AF_INET,(datagram?SOCK_DGRAM:SOCK_STREAM)|SOCK_CLOEXEC,0);
    if(fd<0) fail("socket");
    return Fd(fd);
}
sockaddr_in bind_loopback(int fd) {
    sockaddr_in address{}; address.sin_family=AF_INET; address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(bind(fd,reinterpret_cast<sockaddr*>(&address),sizeof(address))<0) fail("bind loopback");
    socklen_t length=sizeof(address);
    if(getsockname(fd,reinterpret_cast<sockaddr*>(&address),&length)<0) fail("getsockname");
    return address;
}
void connect_to(int fd,const sockaddr_in& address) {
    if(connect(fd,reinterpret_cast<const sockaddr*>(&address),sizeof(address))<0) fail("connect loopback");
}
}
Fd::~Fd() { if(value_>=0) close(value_); }
Fd& Fd::operator=(Fd&& other) noexcept {
    if(this!=&other) { if(value_>=0) close(value_); value_=std::exchange(other.value_,-1); }
    return *this;
}
Deadline deadline(unsigned timeout_ms) { return std::chrono::steady_clock::now()+std::chrono::milliseconds(timeout_ms); }
Pair loopback_pair(bool datagram,bool nodelay) {
    auto client=socket_fd(datagram), server=socket_fd(datagram);
    const auto address=bind_loopback(server.get());
    if(datagram) {
        const auto client_address=bind_loopback(client.get());
        connect_to(client.get(),address); connect_to(server.get(),client_address);
    } else {
        if(listen(server.get(),1)<0) fail("listen");
        connect_to(client.get(),address);
        int accepted=accept4(server.get(),nullptr,nullptr,SOCK_CLOEXEC);
        if(accepted<0) fail("accept");
        server=Fd(accepted);
        int enabled=nodelay?1:0;
        for(int fd:{client.get(),server.get()})
            if(setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&enabled,sizeof(enabled))<0) fail("TCP_NODELAY");
    }
    return {std::move(client),std::move(server)};
}
Channel::Channel(int fd,Mode mode,unsigned timeout_ms):fd_(fd),mode_(mode) {
    if(timeout_ms==0) throw std::invalid_argument("positive network timeout required");
    int flags=fcntl(fd_,F_GETFL);
    if(flags<0 || fcntl(fd_,F_SETFL,mode==Mode::blocking ? flags&~O_NONBLOCK : flags|O_NONBLOCK)<0) fail("fcntl");
    if(mode==Mode::blocking) {
        // Bound each blocking syscall as well as checking the operation deadline.
        timeval timeout{static_cast<time_t>(timeout_ms/1000),static_cast<suseconds_t>((timeout_ms%1000)*1000)};
        for(int option:{SO_RCVTIMEO,SO_SNDTIMEO})
            if(setsockopt(fd_,SOL_SOCKET,option,&timeout,sizeof(timeout))<0) fail("socket timeout");
        blocking_timeout_ms_=timeout_ms;
    } else if(mode!=Mode::busy) {
        epoll_=Fd(epoll_create1(EPOLL_CLOEXEC));
        if(epoll_.get()<0) fail("epoll_create1");
        interest_=EPOLLIN|EPOLLRDHUP|(mode==Mode::epoll_et?static_cast<unsigned>(EPOLLET):0U);
        epoll_event event{}; event.events=interest_; event.data.fd=fd_;
        if(epoll_ctl(epoll_.get(),EPOLL_CTL_ADD,fd_,&event)<0) fail("epoll add");
    }
}
void Channel::check(Deadline end) {
    const auto now=std::chrono::steady_clock::now();
    if(now>=end) throw Timeout();
    if(mode_==Mode::blocking) {
        const auto remaining=std::chrono::ceil<std::chrono::milliseconds>(end-now).count();
        const auto timeout_ms=static_cast<unsigned>(std::clamp<long long>(remaining,1,60000));
        if(timeout_ms!=blocking_timeout_ms_) {
            timeval timeout{static_cast<time_t>(timeout_ms/1000),static_cast<suseconds_t>((timeout_ms%1000)*1000)};
            for(int option:{SO_RCVTIMEO,SO_SNDTIMEO})
                if(setsockopt(fd_,SOL_SOCKET,option,&timeout,sizeof(timeout))<0) fail("remaining socket timeout");
            blocking_timeout_ms_=timeout_ms;
        }
    }
}
void Channel::wait(bool writing,Deadline end) {
    ++counters.eagain;
    check(end);
    if(mode_==Mode::blocking) throw Timeout();
    if(mode_==Mode::busy) { lab::pause_cpu(); return; }
    const auto wanted=(writing?EPOLLOUT:EPOLLIN)|EPOLLRDHUP|(mode_==Mode::epoll_et?static_cast<unsigned>(EPOLLET):0U);
    if(wanted!=interest_) {
        epoll_event event{}; event.events=wanted; event.data.fd=fd_;
        if(epoll_ctl(epoll_.get(),EPOLL_CTL_MOD,fd_,&event)<0) fail("epoll mod");
        interest_=wanted;
    }
    for(;;) {
        check(end);
        auto remaining=std::chrono::ceil<std::chrono::milliseconds>(end-std::chrono::steady_clock::now()).count();
        epoll_event event{}; ++counters.waits;
        int count=epoll_wait(epoll_.get(),&event,1,static_cast<int>(std::clamp<long long>(remaining,1,60000)));
        if(count>0) return; // Retry syscall to report EOF or socket error, including HUP/ERR.
        if(count==0) throw Timeout();
        if(errno!=EINTR) fail("epoll_wait");
    }
}
void Channel::send_all(std::span<const std::byte> bytes,Deadline end) {
    while(!bytes.empty()) {
        check(end); ++counters.sends;
        auto n=send(fd_,bytes.data(),bytes.size(),MSG_NOSIGNAL);
        if(n>0) { if(static_cast<std::size_t>(n)<bytes.size()) ++counters.partial; bytes=bytes.subspan(n); }
        else if(n==0) throw PeerClosed();
        else if(errno==EINTR) continue;
        else if(again()) wait(true,end);
        else fail("send");
    }
}
void Channel::receive_all(std::span<std::byte> bytes,Deadline end) {
    while(!bytes.empty()) {
        check(end); ++counters.receives;
        auto n=recv(fd_,bytes.data(),bytes.size(),0);
        if(n>0) { if(static_cast<std::size_t>(n)<bytes.size()) ++counters.partial; bytes=bytes.subspan(n); }
        else if(n==0) throw PeerClosed();
        else if(errno==EINTR) continue;
        else if(again()) wait(false,end);
        else fail("recv");
    }
}
void Channel::send_datagram(std::span<const std::byte> bytes,Deadline end) {
    for(;;) {
        check(end); ++counters.sends;
        auto n=send(fd_,bytes.data(),bytes.size(),MSG_NOSIGNAL);
        if(n>=0) {
            if(static_cast<std::size_t>(n)!=bytes.size()) throw std::runtime_error("partial datagram send");
            return;
        }
        if(errno==EINTR) continue;
        if(again()) wait(true,end); else fail("datagram send");
    }
}
std::size_t Channel::receive_datagram(std::span<std::byte> bytes,Deadline end) {
    for(;;) {
        check(end); ++counters.receives;
        auto n=recv(fd_,bytes.data(),bytes.size(),MSG_TRUNC);
        if(n>=0) {
            if(static_cast<std::size_t>(n)>bytes.size()) throw std::runtime_error("truncated datagram");
            return static_cast<std::size_t>(n);
        }
        if(errno==EINTR) continue;
        if(again()) wait(false,end); else fail("datagram recv");
    }
}
}
