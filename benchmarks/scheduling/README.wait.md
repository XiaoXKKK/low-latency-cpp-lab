# Experiment: Sleep, yield, spin, hybrid

## Question
不同等待策略的迟到尾部与CPU时间如何权衡？

## Hypothesis
spin可能减少睡眠唤醒延迟，也可能浪费CPU并加重调度压力。 这是待验证假设，不是普遍结论。

## Setup
Linux、C++20、Release GCC/Clang；实际硬件、flags、CPU/NUMA、seed在每次campaign的environment/manifest记录。共同规则见 [Phase2方法学](../../docs/phase2_methodology.md)。

## Baseline
sleep

## Variant
yield、spin、hybrid

## Expected hardware behavior
报告wake lateness；主线程CPU时间单独统计；hybrid最后20us自旋。

## Run
CPU编号先与taskset/lscpu核对。下面从仓库根运行；repeat runner可用同样参数，`--variant`可隔离对照。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark wait_strategy --threads 1 --iterations 10000 --interval-ns 100000 --cpu 0 --format json
python3 tools/run_benchmark.py --benchmark wait_strategy --threads 1 --iterations 10000 --interval-ns 100000 --cpu 0 --repeats 3 --perf
```

## Result
本机真实三轮结果见 [Phase2报告](../../docs/phase2_results.md)，原始样本在results/raw/phase2-*；不可用能力记录NOT MEASURED，统计为null。

## perf analysis
perf_interval区间计数与run_perf全进程计数不同。只有有效的actual counters才能算作机制证据；权限拒绝保留错误，不从ns变化反推出特定miss数。

## Explanation
报告wake lateness；主线程CPU时间单独统计；hybrid最后20us自旋。 从单位、sample_kind、timer/setup边界解释结果；批次均值不能当单操作tail，明确哪些变量保持不变。

## When does this optimization help?
有CPU预算且目标间隔适合所选等待策略。

## When might it hurt?
过量自旋、共享机器、长等待或持锁者抢占时。

## Interview Questions
1. wake lateness与总等待时间有什么区别？
2. yield为什么不保证立即重新运行？
3. spin如何改变CPU时间与调度负载？
4. hybrid的20us阈值为什么不能普遍套用？
5. 线程CPU fraction与整机CPU百分比有什么区别？
