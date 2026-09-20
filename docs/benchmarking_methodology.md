# Benchmarking methodology

1. 明确问题和可被证伪的 hypothesis；记录 baseline 与主要变量。
2. 固定 seed、编译配置、CPU 拓扑和 workload；保留环境快照、flags、二进制与源码哈希。
3. setup/first touch/worker creation 在计时外；运行 warmup，独立启动至少三轮，随机化变体顺序。
4. 极短操作采用 batch，结果标记 batch_mean；真实逐次延迟保留时钟开销且报告 p99/p999/max。
5. 记录全部原始样本；均值/中位数/尾部并列，不能用一个最快样本代表系统。
6. perf counter 是解释机制的补充。不可用时只陈述观测结果，cache coherence 等机制保留为假设。
7. 优化一个主要因素，再测；不追求每个 variant 都“获胜”。

## Timing boundaries

使用 monotonic `steady_clock`，前后 compiler barrier。`do_not_optimize` 的 GNU/Clang asm 输入加 memory clobber 防止被测结果无用和内存访问被移走；它不是 CPU 内存屏障。随机输入运行时生成，分支 kernel noinline 并检查反汇编。没有把 volatile 当作并发同步。

serial 每样本包含一次 std::function 调用、两个时钟读数和必要循环。parallel 预建线程，计时包含 start/done barrier 及其调度成本，不包含 thread create/join/affinity。小 batch 时屏障可能主导；增大 batch 并比较稳定性，不能将该结果称作 lock acquisition latency。

SPSC 的 throughput pass 无逐消息计时；独立 instrumented pass 在 enqueue 首次尝试前取时间，到 consumer pop 后停止，包含 backpressure 和驻留时间。消费者记录数据会影响 workload；不是纯 queue primitive latency，也没有解决 open-loop coordinated omission。容量 1024，实际可用 1023。

allocation 是单对象分配+初始化+释放，非 allocator syscall latency，只有一个 outstanding object、热 tcache。100000 样本的 p999 也只有约 100 个上尾观测；相邻样本相关，不能把分位数当成精确 SLA。普通 100 批 p999 接近最大值，只是描述性统计。

## Statistical contract

对排序样本在 `(n-1)*p` 位置线性插值。stddev 是总体标准差。吞吐按 `sum(ops)/sum(ns)` 求得；不对每批 ops/sec 求算术平均。跨轮 summary 取每轮统计值的中位数，而非拼接样本。没有独立同分布或正态假设，没有自动置信区间；报告 round spread、原始数据并按需扩大轮数。

计时器开销单独运行 timer 实验；empty_loop 为控制组，不做易产生负值的固定减法。TSC 仅作为被测读取指令，所有结果使用 ns；core cycles/IPC 只来自获准的 perf counters。

## Reproducibility

`run_benchmark.py` 记录 UTC、environment、命令顺序、seed、源码/二进制 SHA256、CMake cache 与 compile_commands。每次正式测量使用同一 build。首次试运行/失败/修订后重测分目录保存，禁止结果混用。共享机器负载、频率、温度随时间变化，不宣称完全可复现到某个小数。
