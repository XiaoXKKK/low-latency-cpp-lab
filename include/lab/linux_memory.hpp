#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
namespace lab {
class MappedRegion {
    void* reservation_ = nullptr;
    std::size_t reservation_bytes_ = 0;
    void* data_ = nullptr;
    std::size_t bytes_ = 0;
public:
    explicit MappedRegion(std::size_t bytes, bool hugetlb = false, std::size_t alignment = 4096);
    ~MappedRegion();
    MappedRegion(const MappedRegion&) = delete;
    MappedRegion& operator=(const MappedRegion&) = delete;
    void* data() const { return data_; }
    std::size_t size() const { return bytes_; }
    void advise(int advice);
};
struct PageInfo { std::size_t anon_huge_bytes=0, resident_bytes=0, kernel_page_bytes=0; bool exact=false; };
PageInfo page_info(const MappedRegion& region);
std::vector<int> page_nodes(const MappedRegion& region);
void bind_memory(MappedRegion& region, int node);
int cpu_node(int cpu);
std::size_t page_size();
} // namespace lab
