# Experiment: Serialized TSC interval

## Question
空区间与依赖运算的TSC差值是多少？

## Hypothesis
CPUID序列化增加成本；batch增大后固定成本占比可能减小。 这是待验证假设，不是普遍结论。

## Setup
Linux、C++20、Release GCC/Clang；实际硬件、flags、CPU/NUMA、seed在每次campaign的environment/manifest记录。共同规则见 [Phase2方法学](../../docs/phase2_methodology.md)。

## Baseline
empty

## Variant
dependent_chain

## Expected hardware behavior
CPUID serialization、invariant TSC、AUX端点检查；单位ticks并非core cycles。

## Run
CPU编号先与taskset/lscpu核对。下面从仓库根运行；repeat runner可用同样参数，`--variant`可隔离对照。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark tsc_interval --threads 1 --batch 4096 --cpu 0 --format json
python3 tools/run_benchmark.py --benchmark tsc_interval --threads 1 --batch 4096 --cpu 0 --repeats 3 --perf
```

## Result
本机真实三轮结果见 [Phase2报告](../../docs/phase2_results.md)，原始样本在results/raw/phase2-*；不可用能力记录NOT MEASURED，统计为null。

## perf analysis
perf_interval区间计数与run_perf全进程计数不同。只有有效的actual counters才能算作机制证据；权限拒绝保留错误，不从ns变化反推出特定miss数。

## Explanation
CPUID serialization、invariant TSC、AUX端点检查；单位ticks并非core cycles。 从单位、sample_kind、timer/setup边界解释结果；批次均值不能当单操作tail，明确哪些变量保持不变。

## When does this optimization help?
需要研究计时边界和TSC本身时。

## When might it hurt?
未核实时钟属性、跨核同步或误把ticks称cycles时。

## Interview Questions
1. invariant TSC与实际core frequency是什么关系？
2. CPUID序列化如何影响被测区间？
3. 两端TSC_AUX相同能否排除迁出再迁回？
4. 为什么空区间不能机械减成所有实验的真实开销？
5. 怎样用PMU验证core cycles并与TSC ticks区分？
