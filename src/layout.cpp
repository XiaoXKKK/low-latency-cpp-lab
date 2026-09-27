#include "lab/layout.hpp"
namespace lab::layout {
std::uint64_t sum_aos(const Order* orders, std::size_t n) {
    std::uint64_t total = 0;
    for(std::size_t i = 0; i < n; ++i) total += orders[i].price * orders[i].quantity;
    return total;
}
std::uint64_t sum_soa(const std::uint64_t* price, const std::uint32_t* quantity, std::size_t n) {
    std::uint64_t total = 0;
    for(std::size_t i = 0; i < n; ++i) total += price[i] * quantity[i];
    return total;
}
} // namespace lab::layout
