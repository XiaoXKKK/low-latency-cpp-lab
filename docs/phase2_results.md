# Phase 2 首批实测报告

本轮按用户确认完成前四组：测量深化、Cache/Memory、同步调度、NUMA。原Phase1八组保留，新加11组benchmark及ticket lock；网络、数据结构、order book留到下一批。

后续环境更新（2026-09-19）：Clang 已安装，perf 已为当前用户启用并通过实际构建、测试与采集验证，见 [工具链启用记录](toolchain_setup.md)。本文以下保留原 campaign 的环境与测量状态。

## Repository and validation

工程仍位于 `/home/ljw/low-latency-cpp-lab`，原指定目录权限未恢复。原有Phase1数据保留；本轮结果存于 `results/raw/phase2-complete`。本轮共 **50个campaign，423次独立benchmark进程**：{'MEASURED': 392, 'NOT MEASURED': 31}。非编译矩阵实验使用同一个Release二进制，源码/flags/hash/command/seed均在manifest。

Release、ASan+UBSan、TSan五项CTest全部通过。TSan仅测试进程setarch -R；正式Release运行保留ASLR。编译矩阵每个可用profile也独立build/test。见[evidence](evidence/phase2/release-build-test.log)、[review修复](phase2_review.md)、[阶段范围](phase2_scope.md)。

同一台AMD EPYC 7C13双路、256逻辑CPU、两NUMA节点机器；主要对照使用CPU0/1，4线程锁用0/1/2/3。本次NUMA固定访问CPU0，对比memory node0/1。没有更改系统governor/boost/SMT、NUMA balancing或全局hugepage池。完整[环境](evidence/phase2/environment.txt)包含内存状态与页策略。

环境文档副本只规范行尾空白，raw目录保留原始输出。

所有表中数值是**各轮统计值的中位数**，不混合原始样本；样本单位与valid round数同时报告。完整主表见[主campaign summary](evidence/phase2/main-summary.md)。

## Scheduled response versus service time

每变体3轮、每轮10000个任务、100 warmup；interval=20us，service为4096步依赖整数计算。open schedule不因上次完成而后移，背景线程默认也在CPU0。单位ns。

| Variant | response mean | response p99 | service p99 | response p999 |
|---|---:|---:|---:|---:|
| closed_idle | 5516.855 | 11099.000 | 10851.000 | 19015.681 |
| open_idle | 5512.690 | 5586.400 | 5350.000 | 24910.047 |
| open_cpu | 1089062.536 | 4541795.200 | 5660.000 | 5857113.609 |
| open_memory | 1031623.789 | 2977869.650 | 5591.100 | 3010000.328 |

CPU背景负载下，service p99约5.7us而response p99约4.54ms，显示只测服务区间会漏掉计划积压。该背景对照故意把两个线程放同一CPU，不代表所有生产部署。

另测interval=1us，任务数2000。closed_idle重新定下一到达时刻；open_idle维持原计划：

| Model | response mean ns | response p99 ns |
|---|---:|---:|
| closed_idle | 5490.848 | 6725.000 |
| open_idle | 5042994.525 | 9434422.900 |

这是有限的虚拟恒定到达计划，不是独立网络发包器。duration截断时不能声称排空全部潜在到达；没有随机到达/SLA模型。更大response不是简单地把service变慢，须结合排队和调度解释。

## Waiting: lateness versus CPU budget

5000样本/轮、3轮，目标等待100us；单位ns，CPU fraction为单个测量线程占一个逻辑CPU的时间比例。

| Strategy | lateness p50 | p99 | p999 | CPU fraction |
|---|---:|---:|---:|---:|
| sleep | 59336.000 | 156449.920 | 725603.150 | 0.0493 |
| yield | 293.000 | 733.010 | 8692.357 | 0.9997 |
| spin | 52.000 | 92.000 | 11218.491 | 0.9998 |
| hybrid | 39368.000 | 147907.340 | 649591.749 | 0.0601 |

spin/yield消耗接近一个CPU；sleep/hybrid节省CPU但本轮唤醒迟到明显。hybrid固定最后20us自旋没有消除本机sleep overshoot，不能把固定阈值当通用优化。

## Cache: useful bytes, stride and prefetch

8MiB uint64数据，stride以元素计；每轮100个完整pass，3轮。以下ns/access只针对访问的元素，未访问字节不计入带宽。

| Stride | mean ns/access | useful GB/s |
|---:|---:|---:|
| 1 | 0.180 | 44.393 |
| 2 | 0.371 | 21.561 |
| 4 | 0.510 | 15.692 |
| 8 | 0.895 | 8.943 |
| 16 | 1.486 | 5.382 |
| 32 | 1.078 | 7.420 |
| 64 | 1.580 | 5.062 |
| 128 | 1.835 | 4.360 |

索引预取对照使用相同随机置换；index数组另占8MiB，不能把它从working set中忽略。和dependent pointer chase不同，这里load地址可独立准备。

| Variant / distance | mean ns/access |
|---|---:|
| indexed baseline | 1.044 |
| prefetch / 0 | 1.496 |
| prefetch / 4 | 1.376 |
| prefetch / 16 | 1.102 |
| prefetch / 64 | 1.232 |
| prefetch / 256 | 1.381 |

本轮预取并未呈现“距离越大越好”的规律。streaming带宽与随机indexed吞吐衡量不同访问模式；有用GB/s不等于DRAM counter。没有PMU证据时不把差异单独归因于cache miss。

## Allocation and ownership

每sample先分配batch个64B对象，再统一销毁/释放/reset，所有路径相同初始化与touch；3轮，100 samples/轮。单位为batch平均ns/object。

| Batch | malloc | new | arena | pmr monotonic | pmr pool | thread-local arena | preallocated |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 | 31.691 | 36.483 | 10.803 | 14.711 | 53.717 | 4.709 | 10.796 |
| 256 | 31.760 | 17.911 | 4.719 | 11.348 | 31.348 | 7.984 | 8.173 |
| 4096 | 16.118 | 18.708 | 4.973 | 5.619 | 24.254 | 3.652 | 4.862 |

同一个producer分配、两道内部barrier保持一致，仅改释放线程：主campaign batch2048的same-thread平均28.776ns/object，cross-thread平均35.459ns/object。该值包含发布/回收协调；不能当作free syscall或逐次free tail。

arena/PMR的生命周期和容量约束是收益的一部分；thread-local arena只有单owner，不能推广为通用并发allocator。

## Page behavior and actual backing

主campaign为2MiB，100样本/轮、3轮；touch每个base-page offset写一个字节。

| Variant | valid rounds | sample unit | mean | minor faults per round (warmup included) |
|---|---:|---|---:|---:|
| mapping_roundtrip | 3 | ns/mapping | 14903.780 | NOT MEASURED |
| first_touch | 3 | ns/page-touch | 1844.906 | 56320.000 |
| warm_touch | 3 | ns/page-touch | 3.723 | 0.000 |
| thp | 2 | ns/page-touch | 5.346 | 0.000 |
| hugetlb | 0 | ns/page-touch | NOT MEASURED | NOT MEASURED |

THP只有2/3轮通过完整AnonHugePages前后校验，一轮collapse分配失败；不是三个有效重复，不据此声称THP有稳定收益。显式hugetlb三轮均失败。保留失败样本状态，未重新尝试直到“凑齐好看的数字”。first_touch包含首次物理页分配/写fault；warm_touch提前触页，两个实验不是同一个初始状态。

## Synchronization and memory order

单线程atomic RMW的五种memory order成本接近，但store的seq_cst与较弱store不同；这不是relaxed publication安全的证明。每个order为编译期常量，只比较同类操作。实际[atomic汇编](evidence/phase2/atomic-assembly.txt)显示五类RMW都使用lock addq，seq_cst store使用xchg；较弱store仍为普通store指令。正确的双向release/acquire payload握手已由测试覆盖，错误relaxed示例仅在文档说明，不作为可运行基线。

| Operation | mean ns/op |
|---|---:|
| relaxed_rmw | 5.090 |
| acquire_rmw | 5.353 |
| release_rmw | 5.076 |
| acq_rel_rmw | 5.163 |
| seq_cst_rmw | 5.077 |
| relaxed_store | 0.828 |
| release_store | 1.232 |
| seq_cst_store | 3.261 |

4线程、50样本/轮、3轮；临界区依赖计算步数变化：

| Critical steps | mutex ns/op | spinlock ns/op | ticket ns/op |
|---:|---:|---:|---:|
| 0 | 28.326 | 79.169 | 106.219 |
| 32 | 92.898 | 86.436 | 131.400 |
| 256 | 534.530 | 375.493 | 427.034 |

读写锁矩阵固定critical=32，4线程；read-percent改变读写混合：

| Read percent | mutex ns/op | shared_mutex ns/op |
|---:|---:|---:|
| 0 | 93.198 | 114.824 |
| 50 | 63.357 | 655.852 |
| 90 | 36.139 | 549.040 |
| 100 | 26.199 | 56.204 |

两线程同CPU的oversubscription小batch对照也保存于locks-oversubscribed，但batch16很可能在一次调度片内完成，barrier/调度可能主导；它不能证明持续争用时ticket/spinlock的表现，更不能推导公平性或writer tail。ticket提供取号顺序，不意味着更低延迟。

## NUMA: fixed CPU, verified local/remote memory

64MiB，CPU0不变，3轮，每轮3个完整pass +1 warmup。所有base page在前后逐页查询；下面仅比较strict bound variants，唯一主要变化为memory node。

| Memory node | bound random ns/load | bound streaming useful GB/s | valid rounds |
|---:|---:|---:|---:|
| 0 | 77.035 | 20.123 | 3 |
| 1 | 175.586 | 9.326 | 3 |

本轮remote依赖load明显更贵，但这只是所选拓扑/工作集下的观测。first-touch remote random三轮均发现页并非全部在目标node，拒绝输出local/remote统计；可能涉及fallback或自动页迁移，本版本没有跟踪足够证据区分原因。严格绑定结果不能被拿来“补齐”first-touch结果。

小样本full-pass适合描述带宽/平均依赖load成本，不适合p999。前后快照仍不能排除中途短暂页迁移。[NUMA原始汇总](evidence/phase2/numa-1-summary.md)

## Compiler profiles and assembly

GCC各profile单独构建、CTest、三轮测量。cache为1MiB streaming，branch为65536元素固定随机序列。优化级别、native、LTO各自隔离，不混称为同一种优化。

| Compiler / profile | status | streaming ns/access | random branch ns/element |
|---|---|---:|---:|
| /usr/bin/g++ / O0 | MEASURED | 8.169 | 5.366 |
| /usr/bin/g++ / O1 | MEASURED | 0.362 | 3.977 |
| /usr/bin/g++ / O2 | MEASURED | 0.380 | 3.711 |
| /usr/bin/g++ / O3 | MEASURED | 0.196 | 3.882 |
| /usr/bin/g++ / O3-native | MEASURED | 0.154 | 4.143 |
| /usr/bin/g++ / O3-lto | MEASURED | 0.252 | 3.726 |
| clang++ / all | NOT MEASURED | NOT MEASURED | NOT MEASURED |

每个profile保存objdump完整汇编；摘录见[evidence/phase2/compiler-assembly.md](evidence/phase2/compiler-assembly.md)。runtime更快不单独证明vectorization/inline/unrolling机制；结合实际指令与counter可用性解释。Clang未安装，明确NOT MEASURED。

## Unavailable measurements and limits

perf全进程探测与perf_interval均被权限拒绝，IPC/core cycles/HITM/branch-misses **NOT MEASURED**；TSC实验有独立ticks单位，不能用它代替缺失PMU cycles。显式hugetlb池未修改；THP失败与混合NUMA页归属如实保留。

跨轮统计非置信区间，主机没有独占；频率、热状态、cache/TLB、NUMA balancing和其他任务均可影响结果。背景负载是本实验明确构造的部分，其余机器负载只记录不控制。详见[Phase2方法学](phase2_methodology.md)与[measurement pitfalls](measurement_pitfalls.md)。

## Next batch

继续网络localhost TCP/UDP RTT、正确的epoll LT/ET partial I/O；随后容器/AoS-SoA，最后有语义对照和三轮独立优化的order book。本轮不把这些标为已实现。MPSC保留为有正确性证明后再推进的可选项目。
