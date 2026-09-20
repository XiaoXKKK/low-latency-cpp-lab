# Phase 1：实测结果与适用边界

## Repository status

基础框架、八个实验、统一 CLI/JSON/CSV/表格、原始样本、多轮 runner、环境采集、perf 降级、文档和 correctness tests 已实现。Git 初始化为 main；当前交付为工作区文件，没有创建 commit 或远端 PR。原始用户目标目录无写权限，实际工程交付位于 `/home/ljw/low-latency-cpp-lab`。

当前测试：GCC 13.3、CMake 3.31.6；Release 3/3、ASan+UBSan Debug 3/3、TSan Debug 3/3。TSan 默认运行曾报 unexpected memory mapping，保留 [失败记录](evidence/tsan.log)；仅对测试进程使用 setarch -R 后通过。Release 性能测量未关闭 ASLR。Clang 本机未安装，**NOT MEASURED**；代码提供 Clang 的编译选项支持。

## Environment and provenance

AMD EPYC 7C13，2 sockets，128 physical cores / 256 logical CPUs，2 NUMA nodes。运行时 line size=64 B；每核 L1d=32 KiB、L2=512 KiB；lscpu 汇总 L3=512 MiB/16 instances，不能将总数当作每核独享 LLC。默认 governor/boost/SMT 保留。主要多线程测试使用 node0 的不同物理核 CPU0/1；4线程锁矩阵使用0/1/2/3。未独占这些核，main barrier 协调线程保持允许 mask。

完整 [环境](evidence/environment.txt)、[主测量 manifest](evidence/phase1-main-manifest.json)、[测试日志](evidence/release-build-test.log)、[反汇编](evidence/branch-assembly.txt) 可审计。每个 raw campaign 另存环境、源码/二进制 SHA256、完整命令与 seed、CMakeCache、compile_commands、逐样本 JSON。正式数据使用相同的最终 Release 二进制。

全部正式测量共21个campaign、204次独立benchmark进程；每个manifest中的binary SHA256一致。环境文档副本只规范行尾空白，raw目录保留原始输出。

主测量：3轮，seed=42，200 measured samples + 20 warmup，batch=4096，memory size=32 KiB。每轮分别启动进程并随机变体顺序。下面跨轮汇总为“每轮统计量的中位数”，不是合并样本的统计量。

## All eight experiments

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| affinity | pinned | individual_work_unit | 3 | 5985.945 | 5331.000 | 6690.610 | 13096.845 | — |
| affinity | unpinned | individual_work_unit | 3 | 5365.145 | 5320.000 | 5354.610 | 12391.215 | — |
| allocation | malloc_free | individual_allocate_touch_free | 3 | 46.740 | 50.000 | 51.000 | 83.040 | — |
| allocation | new_delete | individual_allocate_touch_free | 3 | 48.350 | 50.000 | 51.000 | 82.239 | — |
| allocation | pool | individual_allocate_touch_free | 3 | 41.625 | 40.000 | 50.000 | 74.030 | — |
| allocation | preallocated | individual_allocate_touch_free | 3 | 31.770 | 30.000 | 41.000 | 48.209 | — |
| branch | random_branch | batch_mean | 3 | 1.023 | 0.974 | 1.235 | 2.507 | 977739479 |
| branch | random_branchless | batch_mean | 3 | 0.657 | 0.614 | 0.768 | 0.925 | 1520960552 |
| branch | sorted_branch | batch_mean | 3 | 0.620 | 0.619 | 0.621 | 0.645 | 1614126762 |
| branch | sorted_branchless | batch_mean | 3 | 0.777 | 0.766 | 0.771 | 3.080 | 1287188141 |
| false_sharing | packed | batch_mean | 3 | 14.549 | 16.008 | 17.698 | 18.096 | 68732103 |
| false_sharing | padded | batch_mean | 3 | 2.892 | 2.741 | 3.335 | 6.077 | 345796120 |
| locks | mutex | batch_mean | 3 | 26.819 | 26.943 | 29.938 | 38.776 | 37286844 |
| locks | spinlock | batch_mean | 3 | 27.557 | 27.950 | 34.303 | 35.062 | 36288519 |
| memory_access | random | batch_mean_dependent_load | 3 | 1.888 | 1.673 | 3.503 | 4.310 | — |
| memory_access | sequential | batch_mean_dependent_load | 3 | 1.750 | 1.624 | 2.478 | 2.979 | — |
| spsc | naive | batch_mean | 3 | 22.392 | 22.887 | 26.038 | 29.343 | 44659789 |
| spsc | naive_latency | individual_message_instrumented | 3 | 9444.681 | 9398.000 | 24697.000 | 38744.000 | — |
| spsc | padded | batch_mean | 3 | 20.262 | 20.267 | 23.037 | 50.180 | 49352837 |
| spsc | padded_latency | individual_message_instrumented | 3 | 62163.514 | 70215.000 | 92978.000 | 95763.000 | — |
| timer | clock_gettime | batch_mean | 3 | 29.004 | 28.800 | 30.530 | 35.280 | 34477551 |
| timer | empty_loop | batch_mean | 3 | 0.339 | 0.333 | 0.340 | 0.793 | 2948831015 |
| timer | lfence_rdtsc | batch_mean | 3 | 27.703 | 27.484 | 29.361 | 34.803 | 36097571 |
| timer | rdtsc_raw | batch_mean | 3 | 11.896 | 11.643 | 14.591 | 15.701 | 84064813 |
| timer | rdtscp_lfence | batch_mean | 3 | 29.956 | 29.744 | 31.700 | 33.961 | 33382550 |
| timer | steady_clock | batch_mean | 3 | 30.676 | 30.390 | 34.578 | 39.965 | 32599239 |

`batch_mean` 和 `batch_mean_dependent_load` 的 p99 只是批次平均值的分布；不是单次 lock/atomic/load 的尾延迟。SPSC throughput 的 ops/sec 是 messages/sec；instrumented latency 是 enqueue-attempt→dequeue 的逐消息时间。affinity 的 ns 是整个 4096步任务。

## Allocation：larger tail sample

每变体每轮100000个单次样本、1000 warmup、3轮、CPU0；含 timer、初始化/触碰/释放。热 tcache、单 outstanding 对象。

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| allocation | malloc_free | individual_allocate_touch_free | 3 | 41.345 | 40.000 | 81.000 | 81.000 | — |
| allocation | new_delete | individual_allocate_touch_free | 3 | 43.751 | 40.000 | 81.000 | 81.000 | — |
| allocation | pool | individual_allocate_touch_free | 3 | 37.619 | 30.000 | 71.000 | 71.000 | — |
| allocation | preallocated | individual_allocate_touch_free | 3 | 35.716 | 30.000 | 70.000 | 71.000 | — |

这些绝对数值与 clock overhead 是同一量级，不能把差值全部归因于 allocator 本体。p999 只是该闭环热对象 workload 的观测，不能推出生产 HFT 尾延迟。

## Memory capacity sweep

每种大小3轮，每轮3个 measured samples + 1 warmup；每样本至少完整遍历 working set 一次，另有计时外 prefault cycle。单位 ns/dependent load。小数组短时样本受频率/噪声影响更大，不能把低样本 p999 当成结论。

| Working set | sequential median-of-means | random median-of-means | random round-mean range |
|---|---:|---:|---:|
| 4 KiB | 1.629 | 3.375 | 1.632–3.385 |
| 32 KiB | 3.362 | 2.979 | 1.712–3.795 |
| 256 KiB | 1.681 | 4.097 | 4.037–4.259 |
| 1 MiB | 1.633 | 12.435 | 12.169–12.507 |
| 4 MiB | 1.633 | 16.722 | 16.626–16.736 |
| 16 MiB | 1.634 | 21.430 | 20.920–21.577 |
| 64 MiB | 1.641 | 72.783 | 70.821–73.127 |
| 256 MiB | 1.641 | 138.359 | 105.497–156.616 |

256 MiB 随机链约138 ns/load，而顺序链约1.64 ns/load。这与局部性/预取/依赖 load 假设相容，但没有 PMU 数据排除 TLB、NUMA、频率等因素；不能据此精确倒推各级 cache latency。小规模数据并非严格单调，原始异常保留。

## Contention matrix

4 workers / CPU0,1,2,3；batch2048，50 samples +10 warmup，3轮。locks=4/2/1 分别每锁1/2/4线程。critical 是临界区依赖整数更新次数。下面为聚合 ns/op，不是锁获取延迟。

| locks | critical steps | mutex mean | spinlock mean |
|---:|---:|---:|---:|
| 4 | 0 | 4.156 | 3.170 |
| 4 | 32 | 12.181 | 12.232 |
| 4 | 256 | 86.667 | 86.097 |
| 2 | 0 | 15.512 | 14.000 |
| 2 | 32 | 52.283 | 37.023 |
| 2 | 256 | 313.472 | 183.105 |
| 1 | 0 | 28.133 | 65.090 |
| 1 | 32 | 89.199 | 80.912 |
| 1 | 256 | 533.886 | 390.864 |

同一锁类型在竞争与持锁时长改变后排序会变化。该矩阵未模拟持锁线程 sleep 或超过CPU数量的worker；不扩展为 oversubscription 的结论。`scripts/run_lock_matrix.sh` 可复测，单独改变 CPU 列表研究额外调度压力。

## Branch pattern length

主测试4096元素固定随机序列容易被预测器学习，因此另做65536元素、200 samples、20 warmup、3轮补充；仍为相同seed的固定输入，不宣称独立伯努利误预测率。

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| branch | random_branch | batch_mean | 3 | 3.726 | 3.708 | 3.870 | 4.139 | 268420064 |
| branch | random_branchless | batch_mean | 3 | 0.605 | 0.602 | 0.707 | 0.893 | 1653169207 |
| branch | sorted_branch | batch_mean | 3 | 0.593 | 0.647 | 0.775 | 0.779 | 1686153989 |
| branch | sorted_branchless | batch_mean | 3 | 0.604 | 0.601 | 0.702 | 0.707 | 1655870259 |

GCC 反汇编确认 branch 仍有数据相关 jbe，branchless 使用 cmovbe；未在本轮研究 vectorization。缺少 branch-misses 时无法量化误预测比例。

## Affinity tail

每轮10000工作单元、100 warmup、3轮：

| Experiment | Variant | Kind | Rounds | mean | p50 | p99 | p99.9 | ops/sec |
|---|---|---|---:|---:|---:|---:|---:|---:|
| affinity | pinned | individual_work_unit | 3 | 5384.743 | 5320.000 | 8428.300 | 12534.010 | — |
| affinity | unpinned | individual_work_unit | 3 | 5362.166 | 5320.000 | 5461.090 | 13115.120 | — |

本轮未观察到 unpinned 的 CPU 编号变化；endpoint sampling 可能漏掉中途迁移。因此这个对照没有展示“减少迁移”的证据；pinned 的 p99 也未优于 unpinned。核负载/频率/调度噪声仍是候选解释。

## Findings and caveats

- false sharing padding 在该两核 relaxed-RMW workload 中提高吞吐约5倍；这只是本机测量，coherency 机制没有 HITM counter 证据。
- 小随机 branch workload 中 branchless 更快，但 sorted 分支相反；数据顺序、pattern 长度和编译器产物必须一起报告。
- SPSC padding 的未插桩吞吐约49.4M messages/sec，naive约44.7M；逐消息插桩 pass 的平均驻留时间却更高。生产者/消费者速率平衡和积压可能解释这一现象，但本版本未记录 occupancy，不能宣称已证明原因。
- mutex 与 spinlock 必须按 workload/线程数/临界区分别比较。主测试没有出现 spinlock 的普遍优势。
- allocation 包含 timer 成本，短样本值有量化现象；100000样本也不是生产 SLA 证据。
- 各 campaign 是按顺序执行，未与其他本项目 benchmark 或编译工作并行；共享主机其他进程不受本项目控制。完整 caveats 见 [measurement pitfalls](measurement_pitfalls.md)。

## perf: NOT MEASURED

已实际尝试10项事件，主campaign每变体均保存 probe：task-clock/context-switches/cpu-migrations/page-faults/cycles/instructions/branches/branch-misses/cache-references/cache-misses。当前 perf_event_paranoid=4，权限拒绝；详见 [原始错误](evidence/perf-denied.json)。IPC、cache miss、HITM、branch miss 数值均 **NOT MEASURED**，无模拟值。

工具现在会区分探测成功与实际组合计数成功；若未来只有部分事件可用，报告 PARTIALLY MEASURED，并保留所有不可用项的原因。

## Next

[Phase 2](phase2.md)：先 background load/open-loop/PMU 区间与编译器对照，再 allocator/cache/scheduling 扩展、实际页放置验证的 NUMA、网络 RTT/epoll、容器/AoS-SoA，最后可校验语义的 order book 三轮优化。继续保持一个主要变量一次修改。
