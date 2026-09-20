#include "lab/benchmark.hpp"
#include <memory>
#include <new>
#include <stdexcept>
namespace lab {
Results false_sharing(const Config& c) {
    if(c.threads<2) throw std::invalid_argument("false_sharing needs --threads >= 2");
    using Counter=std::atomic<std::uint64_t>;
    const auto line=cache_line_size();
    if((line & (line-1)) || line<alignof(Counter)) throw std::runtime_error("unsupported cache line alignment");
    Results rows;
    for(const std::string variant:{"packed","padded"}) {
        if(!selected(c,variant)) continue;
        auto stride=variant=="packed"?sizeof(Counter):line;
        void* storage=::operator new(stride*c.threads,std::align_val_t(line));
        auto deleter=[line](void* p){::operator delete(p,std::align_val_t(line));};
        std::unique_ptr<void,decltype(deleter)> owner(storage,deleter);
        std::vector<Counter*> counters;
        for(unsigned t=0;t<c.threads;++t) counters.push_back(std::construct_at(reinterpret_cast<Counter*>(static_cast<char*>(storage)+t*stride),0));
        auto r=measure_parallel(c,"false_sharing",variant,c.batch*c.threads,[&](unsigned t){
            for(std::size_t i=0;i<c.batch;++i) counters[t]->fetch_add(1,std::memory_order_relaxed);
        });
        const auto expected=(c.warmup+r.samples.size())*c.batch;
        for(auto p:counters) { if(p->load()!=expected) throw std::runtime_error("counter mismatch"); std::destroy_at(p); }
        r.notes+=" Runtime detected line="+std::to_string(line)+" bytes; identical relaxed RMW, only spacing differs.";
        rows.push_back(std::move(r));
    }
    return rows;
}
}
