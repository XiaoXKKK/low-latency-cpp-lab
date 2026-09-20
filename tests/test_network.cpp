#include "lab/network.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <vector>
#define CHECK(value) do { if(!(value)) throw std::runtime_error(#value); } while(false)
using namespace lab::net;
Pair local_pair() {
    int sockets[2]; CHECK(socketpair(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0,sockets)==0);
    return {Fd(sockets[0]),Fd(sockets[1])};
}
void transfer(Mode mode) {
    auto pair=local_pair(); int small=1024;
    CHECK(setsockopt(pair.client.get(),SOL_SOCKET,SO_SNDBUF,&small,sizeof(small))==0);
    Channel writer(pair.client.get(),mode,2000),reader(pair.server.get(),mode,2000);
    std::vector<std::byte> expected(1024*1024),actual(expected.size());
    for(std::size_t i=0;i<expected.size();++i) expected[i]=static_cast<std::byte>((i*71)%251);
    std::exception_ptr error;
    std::thread producer([&] {
        try { writer.send_all(expected,deadline(2000)); shutdown(pair.client.get(),SHUT_WR); }
        catch(...) { error=std::current_exception(); shutdown(pair.client.get(),SHUT_RDWR); }
    });
    try {
        // Force queued data/backpressure; exact reader reconstructs all bytes.
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        reader.receive_all(actual,deadline(2000));
    } catch(...) { shutdown(pair.server.get(),SHUT_RDWR); producer.join(); throw; }
    producer.join(); if(error) std::rethrow_exception(error);
    CHECK(actual==expected);
    if(mode!=Mode::blocking) CHECK(writer.counters.partial>0 && writer.counters.eagain>0);
    std::array<std::byte,1> extra{}; bool eof=false;
    try { reader.receive_all(extra,deadline(50)); } catch(const PeerClosed&) { eof=true; }
    CHECK(eof);
}
void failure_paths(Mode mode) {
    auto pair=local_pair(); Channel reader(pair.server.get(),mode,20);
    std::array<std::byte,32> buffer{};
    bool timed_out=false;
    try { reader.receive_all(buffer,deadline(20)); } catch(const Timeout&) { timed_out=true; }
    CHECK(timed_out);
    // EOF after an incomplete frame must fail, rather than fabricate completion.
    CHECK(send(pair.client.get(),buffer.data(),3,MSG_NOSIGNAL)==3);
    shutdown(pair.client.get(),SHUT_WR);
    bool eof=false;
    try { reader.receive_all(buffer,deadline(50)); } catch(const PeerClosed&) { eof=true; }
    CHECK(eof);
    // A peer that does not read must produce a bounded backpressure timeout.
    auto blocked=local_pair(); int small=1024;
    CHECK(setsockopt(blocked.client.get(),SOL_SOCKET,SO_SNDBUF,&small,sizeof(small))==0);
    Channel writer(blocked.client.get(),mode,20);
    std::vector<std::byte> large(1024*1024); timed_out=false;
    try { writer.send_all(large,deadline(20)); } catch(const Timeout&) { timed_out=true; }
    CHECK(timed_out);
    // Closed peer must raise an exception without terminating via SIGPIPE.
    blocked.server=Fd(); bool disconnected=false;
    try { writer.send_all(buffer,deadline(20)); } catch(const std::runtime_error&) { disconnected=true; }
    CHECK(disconnected);
}
void readiness(Mode mode) {
    auto pair=local_pair(); Channel reader(pair.server.get(),mode,1000);
    std::array<std::byte,16> bytes{};
    // A frame boundary is not an EAGAIN boundary: queued second frame must be
    // read without requiring a fresh edge. Repeat after fully draining the fd.
    for(int round=0;round<3;++round) {
        CHECK(send(pair.client.get(),bytes.data(),bytes.size(),MSG_NOSIGNAL)==16);
        reader.receive_all(std::span(bytes).first(8),deadline(100));
        reader.receive_all(std::span(bytes).last(8),deadline(100));
        bool timed_out=false;
        try { reader.receive_all(bytes,deadline(5)); } catch(const Timeout&) { timed_out=true; }
        CHECK(timed_out);
    }
}
int main() {
    try {
        for(auto mode:{Mode::blocking,Mode::epoll_lt,Mode::epoll_et,Mode::busy}) { transfer(mode); failure_paths(mode); }
        readiness(Mode::epoll_lt); readiness(Mode::epoll_et);
        auto udp=loopback_pair(true,false); Channel sender(udp.client.get(),Mode::epoll_et,1000),receiver(udp.server.get(),Mode::epoll_et,1000);
        std::array<std::byte,32> frame{}; sender.send_datagram(frame,deadline(100));
        bool truncated=false;
        try { receiver.receive_datagram(std::span(frame).first(8),deadline(100)); } catch(const std::runtime_error&) { truncated=true; }
        CHECK(truncated);
        sender.send_datagram(frame,deadline(100)); CHECK(receiver.receive_datagram(frame,deadline(100))==32);
        std::cout<<"network: partial I/O, backpressure, timeout, EOF, SIGPIPE, ET queued frames/rearm, UDP truncation PASS\n";
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
