# Experiment: Streaming, stride and prefetch

## Question
stride和软件预取如何影响同一个working set？

## Hypothesis
更大stride可能降低line利用率；indexed预取可能隐藏部分访问等待，也可能增加流量。 这是待验证假设，不是普遍结论。

## Setup
Linux、C++20、Release GCC/Clang；实际硬件、flags、CPU/NUMA、seed在每次campaign的environment/manifest记录。共同规则见 [Phase2方法学](../../docs/phase2_methodology.md)。

## Baseline
streaming；indexed

## Variant
stride；prefetch（与indexed保持相同置换）

## Expected hardware behavior
streaming可能向量化；index数组也是working set的一部分；有用字节数不等于DRAM流量。

## Run
CPU编号先与taskset/lscpu核对。下面从仓库根运行；repeat runner可用同样参数，`--variant`可隔离对照。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark cache_patterns --threads 1 --size 8388608 --stride 8 --distance 16 --cpu 0 --format json
python3 tools/run_benchmark.py --benchmark cache_patterns --threads 1 --size 8388608 --stride 8 --distance 16 --cpu 0 --repeats 3 --perf
```

## Result
本机真实三轮结果见 [Phase2报告](../../docs/phase2_results.md)，原始样本在results/raw/phase2-*；不可用能力记录NOT MEASURED，统计为null。

## perf analysis
perf_interval区间计数与run_perf全进程计数不同。只有有效的actual counters才能算作机制证据；权限拒绝保留错误，不从ns变化反推出特定miss数。

## Explanation
streaming可能向量化；index数组也是working set的一部分；有用字节数不等于DRAM流量。 从单位、sample_kind、timer/setup边界解释结果；批次均值不能当单操作tail，明确哪些变量保持不变。

## When does this optimization help?
访问模式稳定、prefetch距离可覆盖延迟的候选场景。

## When might it hurt?
带宽已饱和、预取太近/太远，或增加cache污染时。

## Interview Questions
1. stride以元素还是字节计会怎样改变cache line利用率？
2. dependent pointer chase与indexed独立load有何不同？
3. prefetch距离过近和过远分别有什么代价？
4. index数组的footprint为什么必须计入环境？
5. 如何从汇编判断streaming loop是否向量化？
