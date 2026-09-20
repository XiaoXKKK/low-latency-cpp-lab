# Experiment: Malloc vs fixed pool

## Question
热缓存条件下，64-byte 短生命周期对象的分配/初始化/释放延迟如何变化？

## Hypothesis
预分配可能降低变异，但现代 malloc tcache 可能已非常快。 这是待验证预期，不是最终结论。

## Setup
Linux x86-64、C++20、Release、GCC/Clang。固定 seed=42；本机硬件、compiler、flags、线程数和完整命令由 runner 的 environment/manifest/compile_commands 记录。见 [方法学](../../docs/benchmarking_methodology.md)。
## Baseline
malloc/free 与 new/delete；相同 Block 初始化与 touch。

## Variant
单线程固定 pool（容量1024，free-index栈）、preallocated 单对象；setup 不计时。

## Expected hardware behavior
allocator metadata、tcache、可复用对象与 cache locality；本实验不包含跨线程 free 和多 outstanding。

## Run
在仓库根目录执行；CPU 编号需先确认属于当前 allowed mask。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark allocation --threads 1 --iterations 100000 --warmup 1000 --format json
python3 tools/run_benchmark.py --benchmark allocation --repeats 3 --perf
```

第二条用于精确参数实验，第三条使用 runner 默认参数；正式结论必须引用 manifest 中实际参数。每个变体可加 `--variant NAME` 单独运行 perf，避免把不同 workload 混为一个 counter 值。

## Result
真实数据与三轮汇总见 [Phase 1 实测](../../docs/phase1_results.md)。原始样本保存在 results/raw，统计不手工生成；未来硬件运行结果需新建目录。

## perf analysis
事件：cycles/instructions/branches/branch-misses/cache-references/cache-misses/task-clock/context-switches/cpu-migrations/page-faults。当前若权限拒绝，明确为 **NOT MEASURED**；保留错误，不把运行时间变化冒充 PMU 证据。结果文档给出本机实际状态。

## Explanation
从样本单位与计时边界开始解释，再比较同一轮的 baseline/variant。allocator metadata、tcache、可复用对象与 cache locality；本实验不包含跨线程 free 和多 outstanding。 缺少 counters 时，只能把该机制作为推测。单次延迟与批次均值不能混用；SPSC 两类结果分开看。

## When does this optimization help?
对象大小和容量边界已知、能规划生命周期且希望避免热路径系统分配。

## When might it hurt?
真实峰值容量不可控、对象可变大小、跨线程归还；小热对象 malloc 已高效时收益有限。

## Interview Questions
1. p99.9 至少需要多少样本？
2. tcache 为什么缩小 pool 优势？
3. pool 耗尽如何处理？
4. malloc timing 是否等于 syscall latency？
5. 为什么 HFT 通常控制 hot-path allocation？
