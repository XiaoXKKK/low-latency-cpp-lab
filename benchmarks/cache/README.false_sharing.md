# Experiment: False sharing

## Question
线程写不同计数器时，仅改变变量间距是否改变吞吐？

## Hypothesis
padding 可能减少一致性流量，但 footprint 更大且拓扑会改变收益。 这是待验证预期，不是最终结论。

## Setup
Linux x86-64、C++20、Release、GCC/Clang。固定 seed=42；本机硬件、compiler、flags、线程数和完整命令由 runner 的 environment/manifest/compile_commands 记录。见 [方法学](../../docs/benchmarking_methodology.md)。
## Baseline
line-aligned base 上 packed atomic<uint64_t>；relaxed fetch_add。

## Variant
按运行时 cache line 字节数布置各 counter，操作相同。

## Expected hardware behavior
packed 的 RMW 可能反复请求 cache line 写权限；cache-misses 本身不是 HITM 或 coherency 的充分证据。

## Run
在仓库根目录执行；CPU 编号需先确认属于当前 allowed mask。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark false_sharing --threads 2 --cpu 0,1 --iterations 200 --warmup 20 --batch 65536 --format json
python3 tools/run_benchmark.py --benchmark false_sharing --repeats 3 --perf
```

第二条用于精确参数实验，第三条使用 runner 默认参数；正式结论必须引用 manifest 中实际参数。每个变体可加 `--variant NAME` 单独运行 perf，避免把不同 workload 混为一个 counter 值。

## Result
真实数据与三轮汇总见 [Phase 1 实测](../../docs/phase1_results.md)。原始样本保存在 results/raw，统计不手工生成；未来硬件运行结果需新建目录。

## perf analysis
事件：cycles/instructions/branches/branch-misses/cache-references/cache-misses/task-clock/context-switches/cpu-migrations/page-faults。当前若权限拒绝，明确为 **NOT MEASURED**；保留错误，不把运行时间变化冒充 PMU 证据。结果文档给出本机实际状态。

## Explanation
从样本单位与计时边界开始解释，再比较同一轮的 baseline/variant。packed 的 RMW 可能反复请求 cache line 写权限；cache-misses 本身不是 HITM 或 coherency 的充分证据。 缺少 counters 时，只能把该机制作为推测。单次延迟与批次均值不能混用；SPSC 两类结果分开看。

## When does this optimization help?
独立热点计数器由不同物理核更新，且原本位于同一 line。

## When might it hurt?
字段读局部性重要、对象数量巨大、padding 增加 cache/TLB footprint 时。

## Interview Questions
1. 不同 atomic 为什么还会竞争？
2. relaxed 能否消除 coherency？
3. 怎么检测 line size？
4. HITM 与 cache miss 是同一概念吗？
5. 同核 SMT 与跨 socket 会怎样改变实验？
