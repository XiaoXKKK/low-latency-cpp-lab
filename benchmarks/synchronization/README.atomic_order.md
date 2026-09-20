# Experiment: Atomic order semantics and costs

## Question
相同atomic操作的memory order变化是否改变生成代码和成本？

## Hypothesis
x86不同RMW内存序可能生成相同LOCK指令；store的seq_cst可能不同。 这是待验证假设，不是普遍结论。

## Setup
Linux、C++20、Release GCC/Clang；实际硬件、flags、CPU/NUMA、seed在每次campaign的environment/manifest记录。共同规则见 [Phase2方法学](../../docs/phase2_methodology.md)。

## Baseline
relaxed_rmw、relaxed_store

## Variant
acquire/release/acq_rel/seq_cst RMW；release/seq_cst store

## Expected hardware behavior
模板参数让order成为编译期常量，避免动态order被保守强化。

## Run
CPU编号先与taskset/lscpu核对。下面从仓库根运行；repeat runner可用同样参数，`--variant`可隔离对照。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark atomic_order --threads 1 --batch 65536 --cpu 0 --format json
python3 tools/run_benchmark.py --benchmark atomic_order --threads 1 --batch 65536 --cpu 0 --repeats 3 --perf
```

## Result
本机真实三轮结果见 [Phase2报告](../../docs/phase2_results.md)，原始样本在results/raw/phase2-*；不可用能力记录NOT MEASURED，统计为null。

## perf analysis
perf_interval区间计数与run_perf全进程计数不同。只有有效的actual counters才能算作机制证据；权限拒绝保留错误，不从ns变化反推出特定miss数。

## Explanation
模板参数让order成为编译期常量，避免动态order被保守强化。 从单位、sample_kind、timer/setup边界解释结果；批次均值不能当单操作tail，明确哪些变量保持不变。

## When does this optimization help?
在已有happens-before证明后比较合法实现。

## When might it hurt?
因同样汇编或更快就把publication改成relaxed时。

## Interview Questions
1. 哪些memory order可用于store，哪些用于RMW？
2. 为什么order必须成为编译期常量才能公平比较？
3. x86不同RMW order可能生成相同指令吗？
4. release/acquire必须读到哪个值才建立同步？
5. payload是普通内存时全部relaxed会发生什么？
