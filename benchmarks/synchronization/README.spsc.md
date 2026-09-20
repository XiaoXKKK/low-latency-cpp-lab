# Experiment: SPSC ring buffer

## Question
正确性保持相同时，分离 producer/consumer 游标是否改变吞吐和消息延迟？

## Hypothesis
游标 padding 可能减少 false sharing，但 slot data 和队列占用也影响成本。 这是待验证预期，不是最终结论。

## Setup
Linux x86-64、C++20、Release、GCC/Clang。固定 seed=42；本机硬件、compiler、flags、线程数和完整命令由 runner 的 environment/manifest/compile_commands 记录。见 [方法学](../../docs/benchmarking_methodology.md)。
## Baseline
naive 仅指未 padding；仍然是正确 acquire/release 算法。

## Variant
padded 将 head、tail、slots 对齐128 B；运行时确认可隔离本机 line。无额外 cached-cursor 优化。

## Expected hardware behavior
两端对 head/tail 的发布与读取形成两个方向 happens-before；槽位本身只有在取得所有权时读写。

## Run
在仓库根目录执行；CPU 编号需先确认属于当前 allowed mask。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark spsc --threads 2 --cpu 0,1 --iterations 200 --warmup 20 --batch 4096 --format json
python3 tools/run_benchmark.py --benchmark spsc --repeats 3 --perf
```

第二条用于精确参数实验，第三条使用 runner 默认参数；正式结论必须引用 manifest 中实际参数。每个变体可加 `--variant NAME` 单独运行 perf，避免把不同 workload 混为一个 counter 值。

## Result
真实数据与三轮汇总见 [Phase 1 实测](../../docs/phase1_results.md)。原始样本保存在 results/raw，统计不手工生成；未来硬件运行结果需新建目录。

## perf analysis
事件：cycles/instructions/branches/branch-misses/cache-references/cache-misses/task-clock/context-switches/cpu-migrations/page-faults。当前若权限拒绝，明确为 **NOT MEASURED**；保留错误，不把运行时间变化冒充 PMU 证据。结果文档给出本机实际状态。

## Explanation
从样本单位与计时边界开始解释，再比较同一轮的 baseline/variant。两端对 head/tail 的发布与读取形成两个方向 happens-before；槽位本身只有在取得所有权时读写。 缺少 counters 时，只能把该机制作为推测。单次延迟与批次均值不能混用；SPSC 两类结果分开看。

## When does this optimization help?
恰好一个 producer 和一个 consumer、固定容量、无需热路径 allocation 的消息通道。

## When might it hurt?
多个 producer/consumer 不满足算法前提；忙等消耗 CPU、backpressure 使 tail 增大。

## Interview Questions
1. 为什么容量1024只放1023个消息？
2. head/tail 哪些读可以 relaxed？
3. consumer 释放 tail 为什么同样重要？
4. padding 是否能替代 memory order？
5. 如何区分 messages/sec、ns/message 与 p99 latency？
