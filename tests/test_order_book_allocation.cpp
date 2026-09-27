#include "lab/order_book.hpp"
#include "lab/order_book_vector.hpp"
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

namespace { long fail_after = -1; }
void* operator new(std::size_t size) {
    if(fail_after == 0) throw std::bad_alloc();
    if(fail_after > 0) --fail_after;
    if(auto* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
using namespace lab::book;
#define CHECK(value) do { if(!(value)) throw std::runtime_error(#value); } while(false)
template<class Book> void check(Event target) {
    unsigned failures = 0;
    for(long allowance = 0; allowance < 32; ++allowance) {
        Book book({1, 200, 16});
        std::vector<Trade> trades(16);
        book.apply({Kind::add, 1, Side::buy, 90, 5}, trades);
        book.apply({Kind::add, 2, Side::sell, 100, 5}, trades);
        const auto before = book.snapshot();
        fail_after = allowance;
        bool threw = false;
        try { book.apply(target, trades); } catch(const std::bad_alloc&) { threw = true; }
        fail_after = -1;
        CHECK(book.invariant());
        if(threw) {
            ++failures;
            CHECK(book.snapshot() == before);
        } else {
            CHECK(failures > 0);
            return;
        }
    }
    throw std::runtime_error("operation never completed");
}
int main() {
    try {
        const auto test = []<class Book>() {
            check<Book>({Kind::add, 3, Side::buy, 101, 10}); // Allocation before crossing.
            check<Book>({Kind::modify, 1, Side::buy, 101, 10}); // Directory growth before crossing.
            check<Book>({Kind::modify, 1, Side::buy, 90, 10}); // Same-price increase, stable FIFO.
        };
        test.template operator()<MapBook>();
        test.template operator()<VectorFrontBook>();
        test.template operator()<VectorBackBook>();
        test.template operator()<BranchlessBook>();
        test.template operator()<LinearBook>();
        std::cout << "Allocation failure at every allocation before Add/Modify: unchanged logical state PASS\n";
    } catch(const std::exception& e) { fail_after = -1; std::cerr << e.what() << '\n'; return 1; }
}
