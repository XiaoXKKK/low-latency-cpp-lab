#include "lab/benchmark.hpp"
#include "lab/structures.hpp"
#include <cstdlib>
#include <memory>
namespace lab {
Results allocation(const Config& c) {
    require_threads(c,1); Results rows;
    FixedPool pool(1024); Block preallocated{}; std::uint64_t sequence=0;
    auto add=[&](const std::string& name, auto allocate, auto release) {
        if(!selected(c,name)) return;
        auto r=measure(c,"allocation",name,1,[&] {
            Block* p=allocate();
            *p=Block{}; p->words[0]=++sequence;
            do_not_optimize(p); do_not_optimize(p->words[0]); release(p);
        },"latency","individual_allocate_touch_free");
        r.notes="64-byte object, one outstanding allocation; includes identical zero-init/touch and two timer reads. Warm allocator/tcache. --batch ignored; no overhead subtraction; setup and variant selection outside timing.";
        rows.push_back(std::move(r));
    };
    add("malloc_free",[]{
        void* raw=std::malloc(sizeof(Block)); if(!raw) throw std::bad_alloc();
        return ::new(raw) Block; // trivial default-initialization; common zero-init below
    },[](Block* p){std::destroy_at(p);std::free(p);});
    add("new_delete",[]{return new Block;},[](Block* p){delete p;});
    add("pool",[&]{return pool.allocate();},[&](Block* p){pool.deallocate(p);});
    add("preallocated",[&]{return &preallocated;},[](Block*){});
    return rows;
}
}
