#include "lab/linux_memory.hpp"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <linux/mempolicy.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
namespace lab {
namespace {
std::runtime_error error(const std::string& operation) { return std::runtime_error(operation+": "+std::strerror(errno)); }
}
std::size_t page_size() {
    const auto value=sysconf(_SC_PAGESIZE);
    if(value<=0) throw std::runtime_error("invalid page size");
    return static_cast<std::size_t>(value);
}
MappedRegion::MappedRegion(std::size_t bytes,bool hugetlb,std::size_t alignment) {
    const auto page=page_size(); bytes_=(bytes+page-1)/page*page;
    if(bytes_==0) throw std::invalid_argument("zero mapping");
    if(hugetlb) {
        constexpr std::size_t huge=2*1024*1024;
        if(bytes_%huge) throw std::invalid_argument("hugetlb size must be multiple of 2 MiB");
        reservation_bytes_=bytes_;
        reservation_=mmap(nullptr,bytes_,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|(21<<MAP_HUGE_SHIFT),-1,0);
        if(reservation_==MAP_FAILED) { reservation_=nullptr; throw error("MAP_HUGETLB 2MiB"); }
        data_=reservation_; return;
    }
    if(alignment<page) alignment=page;
    if((alignment&(alignment-1)) || alignment%page) throw std::invalid_argument("mapping alignment");
    // Guard VMAs prevent smaps coalescing with an unrelated anonymous mapping.
    reservation_bytes_=bytes_+alignment+2*page;
    reservation_=mmap(nullptr,reservation_bytes_,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(reservation_==MAP_FAILED) { reservation_=nullptr; throw error("mmap"); }
    const auto start=reinterpret_cast<std::uintptr_t>(reservation_)+page;
    data_=reinterpret_cast<void*>((start+alignment-1)&~(alignment-1));
    if(mprotect(data_,bytes_,PROT_READ|PROT_WRITE)!=0) {
        auto failure=error("mprotect"); munmap(reservation_,reservation_bytes_); reservation_=nullptr; throw failure;
    }
}
MappedRegion::~MappedRegion() { if(reservation_) munmap(reservation_,reservation_bytes_); }
void MappedRegion::advise(int advice) { if(madvise(data_,bytes_,advice)!=0) throw error("madvise"); }
PageInfo page_info(const MappedRegion& region) {
    std::ifstream input("/proc/self/smaps");
    if(!input) throw std::runtime_error("cannot read /proc/self/smaps");
    const auto begin=reinterpret_cast<std::uintptr_t>(region.data()), end=begin+region.size();
    PageInfo result; bool active=false; std::size_t covered=0;
    std::string line;
    while(std::getline(input,line)) {
        unsigned long lo=0,hi=0;
        if(std::sscanf(line.c_str(),"%lx-%lx",&lo,&hi)==2) {
            active=lo>=begin && hi<=end && lo<end && hi>begin;
            if(active) covered+=hi-lo;
            continue;
        }
        if(!active) continue;
        std::size_t kb=0;
        if(std::sscanf(line.c_str(),"AnonHugePages: %zu kB",&kb)==1) result.anon_huge_bytes+=kb*1024;
        if(std::sscanf(line.c_str(),"Rss: %zu kB",&kb)==1) result.resident_bytes+=kb*1024;
        if(std::sscanf(line.c_str(),"KernelPageSize: %zu kB",&kb)==1) result.kernel_page_bytes=kb*1024;
    }
    result.exact=covered==region.size(); return result;
}
std::vector<int> page_nodes(const MappedRegion& region) {
    std::vector<void*> pages;
    for(std::size_t offset=0;offset<region.size();offset+=page_size()) pages.push_back(static_cast<char*>(region.data())+offset);
    std::vector<int> nodes(pages.size(),-1);
    if(syscall(SYS_move_pages,0,pages.size(),pages.data(),nullptr,nodes.data(),0)<0) throw error("move_pages query");
    for(int node:nodes) if(node<0) throw std::runtime_error("move_pages per-page status="+std::to_string(node));
    return nodes;
}
void bind_memory(MappedRegion& region,int node) {
    if(node<0 || node>=1024) throw std::invalid_argument("NUMA node outside supported mask");
    constexpr std::size_t bits=8*sizeof(unsigned long);
    std::vector<unsigned long> mask(static_cast<std::size_t>(node)/bits+1,0);
    mask[static_cast<std::size_t>(node)/bits] |= 1UL << (static_cast<unsigned>(node)%bits);
    // Linux get_nodes() decrements maxnode before copying the bitmap.
    // Pass the allocated bitmap bit capacity + 1, matching libnuma's ABI usage.
    const auto maxnode=mask.size()*bits+1;
    if(syscall(SYS_mbind,region.data(),region.size(),MPOL_BIND,mask.data(),maxnode,MPOL_MF_STRICT)<0) throw error("mbind MPOL_BIND");
}
int cpu_node(int cpu) {
    const auto dir=std::filesystem::path("/sys/devices/system/cpu")/("cpu"+std::to_string(cpu));
    for(const auto& entry:std::filesystem::directory_iterator(dir)) {
        auto name=entry.path().filename().string();
        if(name.starts_with("node") && name.size()>4) return std::stoi(name.substr(4));
    }
    throw std::runtime_error("CPU NUMA node unavailable");
}
}
