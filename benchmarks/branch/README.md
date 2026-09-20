# Experiment: Branch prediction

## Question
相同数值的排序顺序，与 branch/branchless 两种 scalar kernel 如何交互？

## Hypothesis
排序分支更易预测；branchless 对随机输入可能有优势，但也可能做更多工作。 这是待验证预期，不是最终结论。

## Setup
Linux x86-64、C++20、Release、GCC/Clang。固定 seed=42；本机硬件、compiler、flags、线程数和完整命令由 runner 的 environment/manifest/compile_commands 记录。见 [方法学](../../docs/benchmarking_methodology.md)。
## Baseline
sorted_branch；数据为 seed 生成的 0..255，阈值 127。

## Variant
random_branch、sorted_branchless、random_branchless；2×2 设计，每次比较一条轴。

## Expected hardware behavior
误预测和 pipeline recovery 可能解释差异，必须用汇编和 branch-misses 验证。GNU 禁 if-conversion/vectorization；分支 arm 有编译器 barrier，仍需解释这一控制。

## Run
在仓库根目录执行；CPU 编号需先确认属于当前 allowed mask。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark branch --threads 1 --iterations 200 --warmup 20 --batch 65536 --format json
python3 tools/run_benchmark.py --benchmark branch --repeats 3 --perf
```

第二条用于精确参数实验，第三条使用 runner 默认参数；正式结论必须引用 manifest 中实际参数。每个变体可加 `--variant NAME` 单独运行 perf，避免把不同 workload 混为一个 counter 值。

## Result
真实数据与三轮汇总见 [Phase 1 实测](../../docs/phase1_results.md)。原始样本保存在 results/raw，统计不手工生成；未来硬件运行结果需新建目录。

## perf analysis
事件：cycles/instructions/branches/branch-misses/cache-references/cache-misses/task-clock/context-switches/cpu-migrations/page-faults。当前若权限拒绝，明确为 **NOT MEASURED**；保留错误，不把运行时间变化冒充 PMU 证据。结果文档给出本机实际状态。

## Explanation
从样本单位与计时边界开始解释，再比较同一轮的 baseline/variant。误预测和 pipeline recovery 可能解释差异，必须用汇编和 branch-misses 验证。GNU 禁 if-conversion/vectorization；分支 arm 有编译器 barrier，仍需解释这一控制。 缺少 counters 时，只能把该机制作为推测。单次延迟与批次均值不能混用；SPSC 两类结果分开看。

## When does this optimization help?
真实分支不可预测、被跳过计算很少的热点；先观察真实生成代码。

## When might it hurt?
可预测分支、昂贵的非选中计算、更长依赖链或编译器本已生成 cmov 时。

## Interview Questions
1. branchless 为什么可能更慢？
2. 编译器会把 if 变成什么？
3. 为什么 scalar 与 SIMD 要分开比较？
4. 随机短数组会被预测器学会吗？
5. perf branch-misses 包不包含循环分支？
