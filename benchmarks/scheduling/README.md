# Experiment: Thread affinity

## Question
相同工作单元在允许迁核与固定 CPU 时 jitter 是否改变？

## Hypothesis
pinned 可能减少迁移成本，但绑到忙核也可能增加 tail。 这是待验证预期，不是最终结论。

## Setup
Linux x86-64、C++20、Release、GCC/Clang。固定 seed=42；本机硬件、compiler、flags、线程数和完整命令由 runner 的 environment/manifest/compile_commands 记录。见 [方法学](../../docs/benchmarking_methodology.md)。
## Baseline
unpinned 保留启动时 allowed mask。

## Variant
pinned 指定 cpu；每个工作单元由 sched_getcpu 与 batch 次依赖整数运算组成。

## Expected hardware behavior
scheduler、core-local cache、频率和负载影响尾部；观察 CPU 编号变化不是完整 migration tracing。

## Run
在仓库根目录执行；CPU 编号需先确认属于当前 allowed mask。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark affinity --threads 1 --cpu 0 --iterations 10000 --warmup 100 --batch 4096 --format json
python3 tools/run_benchmark.py --benchmark affinity --repeats 3 --perf
```

第二条用于精确参数实验，第三条使用 runner 默认参数；正式结论必须引用 manifest 中实际参数。每个变体可加 `--variant NAME` 单独运行 perf，避免把不同 workload 混为一个 counter 值。

## Result
真实数据与三轮汇总见 [Phase 1 实测](../../docs/phase1_results.md)。原始样本保存在 results/raw，统计不手工生成；未来硬件运行结果需新建目录。

## perf analysis
事件：cycles/instructions/branches/branch-misses/cache-references/cache-misses/task-clock/context-switches/cpu-migrations/page-faults。当前若权限拒绝，明确为 **NOT MEASURED**；保留错误，不把运行时间变化冒充 PMU 证据。结果文档给出本机实际状态。

## Explanation
从样本单位与计时边界开始解释，再比较同一轮的 baseline/variant。scheduler、core-local cache、频率和负载影响尾部；观察 CPU 编号变化不是完整 migration tracing。 缺少 counters 时，只能把该机制作为推测。单次延迟与批次均值不能混用；SPSC 两类结果分开看。

## When does this optimization help?
可为关键线程选择已知拓扑与负载的核心，且有可测迁核/jitter 问题。

## When might it hurt?
绑到繁忙 CPU/SMT sibling、破坏调度器负载均衡、错配 NUMA 内存或 NIC locality 时。

## Interview Questions
1. affinity 和 CPU isolation 有何区别？
2. 零观测迁移能否证明零调度干扰？
3. 外层 taskset 如何破坏 unpinned 对照？
4. 如何选 SMT sibling 之外的 CPU？
5. NIC/CPU/memory 为什么要联合考虑 NUMA？
