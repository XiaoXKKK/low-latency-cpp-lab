#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace lab::layout {
// Integer price ticks avoid floating-point reassociation changing semantics.
struct Order { std::uint64_t price; std::uint32_t quantity, id; char side; };
struct SoA {
    std::vector<std::uint64_t> price;
    std::vector<std::uint32_t> quantity, id;
    std::vector<char> side;
};
// Out-of-line kernels keep inspectable assembly with the same arithmetic.
std::uint64_t sum_aos(const Order* orders, std::size_t n);
std::uint64_t sum_soa(const std::uint64_t* price, const std::uint32_t* quantity, std::size_t n);
} // namespace lab::layout
