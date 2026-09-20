# Experiment: Mutex and shared mutex

## Question
读多写少时共享锁是否改善总吞吐？

## Hypothesis
并行读可能有利，读者登记和writer竞争也可能更贵。 这是待验证假设，不是普遍结论。

## Setup
Linux、C++20、Release GCC/Clang；实际硬件、flags、CPU/NUMA、seed在每次campaign的environment/manifest记录。共同规则见 [Phase2方法学](../../docs/phase2_methodology.md)。

## Baseline
mutex

## Variant
shared_mutex

## Expected hardware behavior
固定read-percent；writer持锁修改共享state；最终写入数验证。

## Run
CPU编号先与taskset/lscpu核对。下面从仓库根运行；repeat runner可用同样参数，`--variant`可隔离对照。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark rw_locks --threads 4 --read-percent 90 --critical 32 --cpu 0,1,2,3 --format json
python3 tools/run_benchmark.py --benchmark rw_locks --threads 4 --read-percent 90 --critical 32 --cpu 0,1,2,3 --repeats 3 --perf
```

## Result
本机真实三轮结果见 [Phase2报告](../../docs/phase2_results.md)，原始样本在results/raw/phase2-*；不可用能力记录NOT MEASURED，统计为null。

## perf analysis
perf_interval区间计数与run_perf全进程计数不同。只有有效的actual counters才能算作机制证据；权限拒绝保留错误，不从ns变化反推出特定miss数。

## Explanation
固定read-percent；writer持锁修改共享state；最终写入数验证。 从单位、sample_kind、timer/setup边界解释结果；批次均值不能当单操作tail，明确哪些变量保持不变。

## When does this optimization help?
真实读写比例、临界区足够大且读者可并行的场景。

## When might it hurt?
短读段、writer饥饿、优先级或公平性要求不满足时。

## Interview Questions
1. 共享读锁为什么未必比mutex快？
2. writer的饥饿或tail能否从总吞吐看出？
3. read-percent与critical section长度如何影响对照？
4. 库实现的reader/writer公平策略是否有标准保证？
5. 读者检查state时为什么仍然需要同步？
