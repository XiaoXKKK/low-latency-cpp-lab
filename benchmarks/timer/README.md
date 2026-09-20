# Experiment: Timer overhead

## Question
不同计时 API 与 TSC 读取序列本身有多大开销？

## Hypothesis
加 fence 的 TSC 读取可能比 raw RDTSC 更贵；API 成本依 vDSO/硬件而变。 这是待验证预期，不是最终结论。

## Setup
Linux x86-64、C++20、Release、GCC/Clang。固定 seed=42；本机硬件、compiler、flags、线程数和完整命令由 runner 的 environment/manifest/compile_commands 记录。见 [方法学](../../docs/benchmarking_methodology.md)。
## Baseline
empty_loop 控制组；steady_clock API。

## Variant
clock_gettime、rdtsc_raw、lfence_rdtsc、rdtscp_lfence；CPUID feature gate。

## Expected hardware behavior
RDTSC 可能乱序；序列化改变执行重叠。chrono/clock_gettime 在 Linux 可能走 vDSO，无需每次 syscall。

## Run
在仓库根目录执行；CPU 编号需先确认属于当前 allowed mask。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark timer --threads 1 --iterations 1000 --warmup 100 --batch 4096 --format json
python3 tools/run_benchmark.py --benchmark timer --repeats 3 --perf
```

第二条用于精确参数实验，第三条使用 runner 默认参数；正式结论必须引用 manifest 中实际参数。每个变体可加 `--variant NAME` 单独运行 perf，避免把不同 workload 混为一个 counter 值。

## Result
真实数据与三轮汇总见 [Phase 1 实测](../../docs/phase1_results.md)。原始样本保存在 results/raw，统计不手工生成；未来硬件运行结果需新建目录。

## perf analysis
事件：cycles/instructions/branches/branch-misses/cache-references/cache-misses/task-clock/context-switches/cpu-migrations/page-faults。当前若权限拒绝，明确为 **NOT MEASURED**；保留错误，不把运行时间变化冒充 PMU 证据。结果文档给出本机实际状态。

## Explanation
从样本单位与计时边界开始解释，再比较同一轮的 baseline/variant。RDTSC 可能乱序；序列化改变执行重叠。chrono/clock_gettime 在 Linux 可能走 vDSO，无需每次 syscall。 缺少 counters 时，只能把该机制作为推测。单次延迟与批次均值不能混用；SPSC 两类结果分开看。

## When does this optimization help?
短操作 overhead 评估与选择批次大小；原始 TSC 只作为成本对照。

## When might it hurt?
未经序列化、迁核或把 invariant ticks 当 core cycles 会破坏解释。

## Interview Questions
1. RDTSC 是否序列化？
2. RDTSCP 与 LFENCE 分别约束什么？
3. 为什么 CPUID 改变测量开销？
4. TSC invariant 是否表示 core frequency 固定？
5. 如何检测跨核与虚拟化影响？
