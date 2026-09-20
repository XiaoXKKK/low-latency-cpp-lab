#include "lab/benchmark.hpp"
#include "lab/linux_memory.hpp"
#include <memory>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/resource.h>
namespace lab {
namespace {
void touch_pages(MappedRegion& region,std::uint64_t& value) {
    auto* bytes=static_cast<unsigned char*>(region.data());
    const auto step=page_size();
    for(std::size_t i=0;i<region.size();i+=step) bytes[i]=static_cast<unsigned char>(++value);
    do_not_optimize(bytes);
}
}
Results page_behavior(const Config& c) {
    require_threads(c,1);
    if(c.size<page_size() || c.size%page_size()) throw std::invalid_argument("page_behavior size must be page multiple");
    Results rows;
    for(const std::string variant:{"mapping_roundtrip","first_touch","warm_touch","thp","hugetlb"}) {
        if(!selected(c,variant)) continue;
        try {
            if(variant=="mapping_roundtrip") {
                auto r=measure(c,"page_behavior",variant,1,[&]{MappedRegion region(c.size); do_not_optimize(region.data());},"latency","individual_guarded_mmap_mprotect_munmap");
                r.notes="Guarded mapping reservation+mprotect+munmap; no page touch. Not a raw mmap syscall-only timer.";
                rows.push_back(std::move(r)); continue;
            }
            const bool huge=variant=="hugetlb", transparent=variant=="thp";
            if((huge || transparent) && c.size%(2*1024*1024)) throw std::invalid_argument("THP/hugetlb size must be multiple of 2 MiB");
            std::unique_ptr<MappedRegion> region;
            std::uint64_t value=c.seed; rusage before{},after{}; double minor=0,major=0;
            auto create=[&]{region=std::make_unique<MappedRegion>(c.size,huge,transparent?2*1024*1024:page_size());
                if(!huge) region->advise(transparent?MADV_HUGEPAGE:MADV_NOHUGEPAGE);
            };
            create();
            PageInfo info_before;
            if(variant!="first_touch") {
                touch_pages(*region,value);
                if(transparent) {
#ifdef MADV_COLLAPSE
                    region->advise(MADV_COLLAPSE);
#else
                    rows.push_back(unavailable(c,"page_behavior",variant,"MADV_COLLAPSE headers unavailable; huge backing unverified")); continue;
#endif
                }
                info_before=page_info(*region);
                if(transparent && (!info_before.exact || info_before.anon_huge_bytes!=region->size())) {
                    rows.push_back(unavailable(c,"page_behavior",variant,"THP actual full backing not verified in smaps")); continue;
                }
            }
            auto r=measure(c,"page_behavior",variant,region->size()/page_size(),[&]{touch_pages(*region,value);},"latency","batch_mean_page_touch",
                [&]{if(variant=="first_touch") create(); if(getrusage(RUSAGE_THREAD,&before)) throw std::runtime_error("getrusage");},
                [&]{if(getrusage(RUSAGE_THREAD,&after)) throw std::runtime_error("getrusage"); minor+=after.ru_minflt-before.ru_minflt; major+=after.ru_majflt-before.ru_majflt;});
            const auto info_after=page_info(*region);
            if(transparent && (!info_after.exact || info_after.anon_huge_bytes!=region->size())) {
                rows.push_back(unavailable(c,"page_behavior",variant,"THP backing changed during measurement")); continue;
            }
            r.metrics={{"minor_faults_including_warmup",minor},{"major_faults_including_warmup",major},
                {"anon_huge_bytes_before",static_cast<double>(info_before.anon_huge_bytes)},{"anon_huge_bytes_after",static_cast<double>(info_after.anon_huge_bytes)},
                {"kernel_page_bytes_after",static_cast<double>(info_after.kernel_page_bytes)}};
            r.notes="One byte write per base-page offset, ns/touch; --batch unused. Mapping/verification outside timed touch. first_touch uses fresh mapping each sample; warm/huge variants prefaulted. THP requires full smaps backing before/after; explicit hugetlb requires successful MAP_HUGETLB. Never changes system hugepage pool.";
            rows.push_back(std::move(r));
        } catch(const std::runtime_error& e) { rows.push_back(unavailable(c,"page_behavior",variant,e.what())); }
    }
    return rows;
}
}
