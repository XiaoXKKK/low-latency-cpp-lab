# Phase 2 roadmap

当前状态：**首批（前四组）已完成实现、测试、review与本机实测**。详见[本轮范围](phase2_scope.md)、[实测报告](phase2_results.md)。perf/Clang/显式hugetlb等不可用能力按原始证据记录；THP部分成功与NUMA不合格页放置也保留。MPSC仍为可选后续工作。

下一批从第5组Networking开始，再推进Data structures和Order book；不重复生成已完成的第一批。

按依赖关系推进，每项延续 question/baseline/单变量/三轮/perf/边界说明。

1. **测量深化**：open-loop 到达模型、background CPU/memory stress、受控 timer bracket、TSC_AUX 迁核过滤、可验证的 cycles、perf 区间计数、跨 GCC/Clang 和 O0/O1/O2/O3/native/LTO 汇编比较。
2. **Cache/Memory**：streaming bandwidth、stride 1..128、cache line utilization、prefetch distance、arena/pmr/thread-local、多 outstanding 分配与跨线程 free、mmap/page faults/THP/显式 hugepages。
3. **Concurrency/Scheduling**：ticket/shared_mutex、relaxed/acquire/release/acq_rel/seq_cst 语义与性能分开；sleep/yield/spin/hybrid 和 CPU utilization；再考虑有正确性证据的 MPSC。
4. **NUMA**：本机两个 node，验证实际 page placement 后测 local/remote 的 sequential bandwidth、random/pointer chase、first touch；记录 automatic balancing 和 affinity。
5. **Networking**：localhost TCP/UDP RTT、NODELAY、batching，随后 nonblocking epoll LT/ET 的 EAGAIN/partial I/O correctness tests；busy polling 及 CPU 消耗；不要求专用网卡。
6. **Data structures / AoS vs SoA**：指定 N=16..100000、lookup/insert/iterate/erase；vector/list/deque/map/unordered_map/sorted vector；汇编验证 vectorization。
7. **Order book**：价格时间优先的可测试语义，Add/Cancel/Modify/Match，synthetic 60/25/10/5 输入；naive map baseline，之后三轮独立优化，每轮真实 before/after/change/why/perf evidence。

验收前不写“lock-free 更快”“hugepage 一定更快”“io_uring 一定胜过 epoll”。没有测到的项目一律 NOT MEASURED。
