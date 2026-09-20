# Experiment: Region perf counters

## Question
排除启动/预热后，代码区间执行了多少cycles/instructions？

## Hypothesis
区间计数有利于归因，但小区间可能由框架控制成本主导。 这是待验证假设，不是普遍结论。

## Setup
Linux、C++20、Release GCC/Clang；实际硬件、flags、CPU/NUMA、seed在每次campaign的environment/manifest记录。共同规则见 [Phase2方法学](../../docs/phase2_methodology.md)。

## Baseline
empty

## Variant
dependent_chain

## Expected hardware behavior
按组同步读取cycles/instructions，保留enabled/running和counter拒绝信息。

## Run
CPU编号先与taskset/lscpu核对。下面从仓库根运行；repeat runner可用同样参数，`--variant`可隔离对照。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark perf_interval --threads 1 --batch 65536 --cpu 0 --format json
python3 tools/run_benchmark.py --benchmark perf_interval --threads 1 --batch 65536 --cpu 0 --repeats 3 --perf
```

## Result
本机真实三轮结果见 [Phase2报告](../../docs/phase2_results.md)，原始样本在results/raw/phase2-*；不可用能力记录NOT MEASURED，统计为null。

## perf analysis
perf_interval区间计数与run_perf全进程计数不同。只有有效的actual counters才能算作机制证据；权限拒绝保留错误，不从ns变化反推出特定miss数。

## Explanation
按组同步读取cycles/instructions，保留enabled/running和counter拒绝信息。 从单位、sample_kind、timer/setup边界解释结果；批次均值不能当单操作tail，明确哪些变量保持不变。

## When does this optimization help?
允许硬件计数器，且被测区间足够长时。

## When might it hurt?
权限不可用、过度multiplexing或过短测量时。

## Interview Questions
1. PERF_FORMAT_GROUP为什么适合同时统计cycles和instructions？
2. RESET为何不重置enabled/running累计时间？
3. multiplexing后raw IPC和cycles/op有什么解释限制？
4. 为什么perf stat全进程与区间计数可能不同？
5. 权限拒绝与真实零counter怎样区分？
