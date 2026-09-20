#include "lab/benchmark.hpp"
#include "lab/structures.hpp"
#include "lab/arena.hpp"
#include <barrier>
#include <cstdlib>
#include <memory_resource>
#include <stdexcept>
namespace lab {
Results allocator_batch(const Config& c) {
    require_threads(c,1);
    if(c.batch>65536) throw std::invalid_argument("allocator batch <=65536 objects");
    std::vector<Block*> pointers(c.batch); std::vector<Block> preallocated(c.batch);
    Arena arena(c.batch*(sizeof(Block)+alignof(Block)));
    thread_local std::unique_ptr<Arena> local_arena;
    local_arena=std::make_unique<Arena>(c.batch*(sizeof(Block)+alignof(Block)));
    std::vector<std::byte> buffer(c.batch*(sizeof(Block)+alignof(Block)));
    std::pmr::monotonic_buffer_resource monotonic(buffer.data(),buffer.size(),std::pmr::null_memory_resource());
    std::pmr::unsynchronized_pool_resource pmr_pool;
    Results rows;
    auto add=[&](const std::string& variant,auto allocate,auto release,auto reset) {
        if(!selected(c,variant)) return;
        std::uint64_t generation=c.seed;
        auto r=measure(c,"allocator_batch",variant,c.batch,[&]{
            std::size_t allocated=0;
            try {
                for(;allocated<c.batch;++allocated) {
                    auto* block=allocate(allocated); pointers[allocated]=block;
                    *block=Block{}; block->words[0]=generation+allocated; do_not_optimize(block);
                }
                for(std::size_t i=0;i<c.batch;++i) { do_not_optimize(pointers[i]->words[0]); release(pointers[i]); }
                reset(); ++generation;
            } catch(...) { for(std::size_t i=0;i<allocated;++i) release(pointers[i]); reset(); throw; }
        });
        r.notes="64-byte objects; --batch simultaneously live before bulk retirement. ns/object includes allocate, zero-init, touch, deallocate/reset. Setup outside timer; arena/pmr monotonic have bounded backing stores. Thread-local arena has one owner, not a general shared allocator.";
        rows.push_back(std::move(r));
    };
    add("malloc",[](std::size_t){ void* raw=std::malloc(sizeof(Block)); if(!raw) throw std::bad_alloc(); return ::new(raw) Block; },[](Block* p){std::destroy_at(p);std::free(p);},[]{});
    add("new",[](std::size_t){return new Block;},[](Block* p){delete p;},[]{});
    add("arena",[&](std::size_t){return ::new(arena.allocate(sizeof(Block),alignof(Block))) Block;},[](Block* p){std::destroy_at(p);},[&]{arena.reset();});
    add("pmr_monotonic",[&](std::size_t){return ::new(monotonic.allocate(sizeof(Block),alignof(Block))) Block;},[](Block* p){std::destroy_at(p);},[&]{monotonic.release();});
    add("pmr_pool",[&](std::size_t){return ::new(pmr_pool.allocate(sizeof(Block),alignof(Block))) Block;},[&](Block* p){std::destroy_at(p);pmr_pool.deallocate(p,sizeof(Block),alignof(Block));},[]{});
    add("thread_local_arena",[&](std::size_t){return ::new(local_arena->allocate(sizeof(Block),alignof(Block))) Block;},[](Block* p){std::destroy_at(p);},[&]{local_arena->reset();});
    add("preallocated",[&](std::size_t i){return &preallocated[i];},[](Block*){},[]{});
    return rows;
}
Results allocation_handoff(const Config& c) {
    require_threads(c,2);
    if(c.batch>65536) throw std::invalid_argument("allocation_handoff batch <=65536");
    Results rows;
    for(const std::string variant:{"same_thread_free","cross_thread_free"}) {
        if(!selected(c,variant)) continue;
        std::vector<void*> pointers(c.batch,nullptr); std::barrier phase(2); std::atomic<bool> failed{false};
        auto r=measure_parallel(c,"allocation_handoff",variant,c.batch,[&](unsigned t){
            if(t==0) for(std::size_t i=0;i<c.batch;++i) {
                pointers[i]=std::malloc(sizeof(Block));
                if(!pointers[i]) failed.store(true,std::memory_order_relaxed);
                else { auto* p=::new(pointers[i]) Block{}; p->words[0]=i; do_not_optimize(p); }
            }
            phase.arrive_and_wait(); // publishes the pointer vector to the freeing thread
            if((variant=="same_thread_free" && t==0) || (variant=="cross_thread_free" && t==1))
                for(auto* p:pointers) { if(p) std::destroy_at(static_cast<Block*>(p)); std::free(p); }
            phase.arrive_and_wait(); // no reuse of pointers until all frees complete
        });
        if(failed.load()) throw std::bad_alloc();
        r.notes+=" Identical allocation producer and two internal barriers; only freeing thread differs. malloc/free throughput with --batch live objects, not per-free tail latency.";
        rows.push_back(std::move(r));
    }
    return rows;
}
}
