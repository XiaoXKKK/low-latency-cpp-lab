# Phase 2 measurement contract

## Samples, status and units

Schema v2 保留 Phase1 的统计字段，增加 `status`、`metrics`，CSV/表格增加相同信息和 notes，共22列。`NOT MEASURED` 的统计/吞吐为 null（CSV为空）、样本为空，原因不为空。不会用零代表权限拒绝、hugepage失败或未验证NUMA放置。

`tsc_ticks/op` 是TSC计数，不是core cycles；普通结果仍是 `ns/op`，精确操作含义由sample_kind给出。streaming指标 `useful_bytes_per_second` 只统计程序有用读取字节，不等于内存控制器DRAM流量。stride未触及的字节不计入有用带宽。

`--stride` 以uint64元素计，`--distance` 是indexed access的预取领先元素数。cache_patterns每样本完整遍历索引集合；memory/NUMA每次至少一个完整pass，不把额外index数组忽略掉。`--size` 控制数据大小，runner为page/NUMA选择可运行默认大小。

## TSC and PMU

两端使用 `CPUID → RDTSCP → CPUID`，保守地序列化指令；差值包含序列化开销。记录TSC_AUX不一致和非单调样本的拒绝数，空区间作为控制组。相同AUX不能排除迁出再迁回；invariant TSC不证明跨socket完全同步。这里没有频率校准后伪装成“实际CPU cycles”。[AMD programmer manuals](https://docs.amd.com/v/u/en-US/24594_3.37)

perf_interval 使用calling-thread cycles/instructions硬件group，预热在enable之外，计时器读数和控制开销在启用区间内。保留raw counts与enabled/running时间；每次RESET清零counts不清零累计时间，因此按相邻读数求时间差。发生multiplexing时不报告cycles/op，raw IPC也需谨慎解释。[perf_event_open](https://www.man7.org/linux/man-pages/man2/perf_event_open.2.html)

编译矩阵依次构建/test/measure，不把编译负载放在测量旁边。O0/O1/O2/O3、O3-native、O3-LTO分别保存flags、哈希、汇编。branch实验仍保留scalar/防if-conversion控制；vectorization以cache streaming的编译产物观察。工具缺失/可选IPO不支持独立记录，真实编译/测试失败仍使脚本最终失败。

## Arrival and wait models

`arrival_latency` 是单服务者、恒定间隔、确定性计划时间的虚拟到达实验。open计划 `begin + (i+1)*interval` 不因完成时间后移；第i个任务的response为计划到达到完成，service为实际开始到完成。closed控制组在上个任务结束后重新定计划，展示闭环隐藏排队成本的风险。

这不是独立发包器或网络服务，也不产生随机到达分布。每次运行至指定样本数或duration上限；只测本次有限计划中的已完成任务，duration截断时不代表排空所有到达。最大积压指标是计划时间差/interval的连续估计，不是实测网络队列深度。service中仍可能包含抢占，response额外包含落后计划的时间。tail无SLA保证。

CPU/memory背景线程在计时前分配、绑核并就绪，默认与测量线程同CPU形成可控竞争；`--background-cpu`可选择其他允许CPU研究共享资源。该线程随实验结束join，不遗留stress进程。后台memory扫描包含读写与整数计算，不能声称是纯DRAM饱和测试。没有修改系统governor、priority或IRQ。

wait_strategy分别sleep/yield/spin/hybrid，报告“目标唤醒时刻后的迟到量”和主线程CPU时间/测量wall时间。hybrid固定最后20us自旋，适用范围依实际sleep精度。CPU fraction是单线程占一个逻辑CPU的时间比例，不是全机器利用率。

## Pages and NUMA

普通mapping两侧保留PROT_NONE guards，使smaps对应被测VMA；mapping_roundtrip包括guard mmap+mprotect+munmap。first_touch每样本fresh mapping；warm_touch提前逐页写。逐页一次byte store，不等于每页吞吐。fault计数包括warmup，计数读取在touch计时外。

THP对齐2MiB，MADV_HUGEPAGE并尝试MADV_COLLAPSE，前后均检查全部VMA的AnonHugePages覆盖；建议成功不代表实际得到hugepages。显式2MiB hugetlb要求MAP_HUGETLB成功，不自动配置池。[Linux THP](https://docs.kernel.org/admin-guide/mm/transhuge.html)

NUMA使用mbind strict policy或者独立first-touch策略；每个base page在前后由move_pages查询归属，混合/改变/无法查询时拒绝作为local/remote结果。bound可在CPU0触碰绑定node1的内存；first-touch默认寻找目标node允许CPU。两者不是同一实验。所有策略只作用于当前mapping，线程affinity结束后恢复。

Linux get_nodes()先递减maxnode，因此raw syscall传入“已分配nodemask位容量+1”，并保证拷贝范围正确。该ABI边界已通过本机绑定+逐页查询回归验证。[Linux 6.8 mempolicy implementation](https://github.com/torvalds/linux/blob/v6.8/mm/mempolicy.c)

即使前后页归属一致，也不能排除期间短暂迁移；缓存驻留和实际DRAM NUMA成本不同。NUMA小working set可能主要命中cache。正式local/remote大工作集对照固定访问CPU和绑定策略，只改memory node，CPU/cache/频率条件仍需环境记录。
