#include "lab/order_book_vector.hpp"
#include "lab/order_book_workload.hpp"
#include <iostream>
using namespace lab::book;
template<bool Back> void run(std::size_t n) {
    const auto trace = make_trace(n, 20000, 42);
    VectorBook<Back, Search::binary, true> book(trace.limits);
    std::vector<Trade> trades(trace.limits.max_orders);
    for(const auto& event : trace.initial) book.apply(event, trades);
    book.reset_diagnostics();
    for(const auto& event : trace.events) book.apply(event, trades);
    if(book.snapshot() != trace.final_state || !book.invariant()) throw std::runtime_error("diagnostic state mismatch");
    const auto stats = book.diagnostics();
    std::cout << (Back ? "vector_back" : "vector_front") << ',' << n << ',' << stats.searches << ','
              << stats.inserts << ',' << stats.erases << ',' << stats.insert_moves << ',' << stats.erase_moves << ','
              << stats.max_capacity_bytes;
    for(auto bucket : stats.best_rank_buckets) std::cout << ',' << bucket;
    std::cout << '\n';
}
int main() {
    std::cout << "variant,initial_orders,searches,inserts,erases,insert_moves,erase_moves,max_directory_bytes,rank0,rank1_3,rank4_15,rank16_63,rank64_plus\n";
    for(auto n : {16U, 256U, 4096U}) { run<false>(n); run<true>(n); }
}
