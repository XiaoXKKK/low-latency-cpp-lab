# Experiment: Sequential vs random memory access

## Question
同样依赖 load，顺序与随机布局随 working set 增大会怎样？

## Hypothesis
随机链可能更难预取；超过 cache/TLB 覆盖后每 load 成本可能增加。 这是待验证预期，不是最终结论。

## Setup
Linux x86-64、C++20、Release、GCC/Clang。固定 seed=42；本机硬件、compiler、flags、线程数和完整命令由 runner 的 environment/manifest/compile_commands 记录。见 [方法学](../../docs/benchmarking_methodology.md)。
容量矩阵：4096、32768、262144、1048576、4194304、16777216、67108864、268435456 字节。每样本执行 max(batch, size/4) 次依赖 load，至少完整覆盖工作集一次；完整 prefault cycle 在计时外。大数组测量可能需数分钟，见 scripts/run_memory_sweep.sh。

## Baseline
4-byte index 的 sequential 单一闭合链。

## Variant
相同 n 个元素随机置换成一个闭合链；只改变连接顺序。

## Expected hardware behavior
硬件预取/空间局部性、依赖链延迟、TLB、cache capacity 共同影响。不是独立 load 的 memory bandwidth。

## Run
在仓库根目录执行；CPU 编号需先确认属于当前 allowed mask。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark memory_access --threads 1 --size 1048576 --iterations 20 --warmup 2 --batch 4096 --format json
python3 tools/run_benchmark.py --benchmark memory_access --repeats 3 --perf
```

第二条用于精确参数实验，第三条使用 runner 默认参数；正式结论必须引用 manifest 中实际参数。每个变体可加 `--variant NAME` 单独运行 perf，避免把不同 workload 混为一个 counter 值。

## Result
真实数据与三轮汇总见 [Phase 1 实测](../../docs/phase1_results.md)。原始样本保存在 results/raw，统计不手工生成；未来硬件运行结果需新建目录。

## perf analysis
事件：cycles/instructions/branches/branch-misses/cache-references/cache-misses/task-clock/context-switches/cpu-migrations/page-faults。当前若权限拒绝，明确为 **NOT MEASURED**；保留错误，不把运行时间变化冒充 PMU 证据。结果文档给出本机实际状态。

## Explanation
从样本单位与计时边界开始解释，再比较同一轮的 baseline/variant。硬件预取/空间局部性、依赖链延迟、TLB、cache capacity 共同影响。不是独立 load 的 memory bandwidth。 缺少 counters 时，只能把该机制作为推测。单次延迟与批次均值不能混用；SPSC 两类结果分开看。

## When does this optimization help?
需要估计依赖数据结构的访存成本时。

## When might it hurt?
将结果推断为 memcpy 吞吐、只采数组一小段却解释为整个 cache 容量时。

## Interview Questions
1. 为什么必须形成单一 permutation cycle？
2. 依赖 load 与 streaming bandwidth 有何不同？
3. 如何区别 TLB miss 与 cache miss？
4. prefault 后是否一定 cache hot？
5. 为什么总 LLC 容量不能直接当每核容量？
