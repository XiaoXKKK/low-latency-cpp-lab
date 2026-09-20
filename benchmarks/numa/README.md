# Experiment: Verified NUMA locality

## Question
实际页放在本地/远端node时，streaming与dependent load怎样变化？

## Hypothesis
超出cache后remote访问可能更贵；small set不一定反映DRAM locality。 这是待验证假设，不是普遍结论。

## Setup
Linux、C++20、Release GCC/Clang；实际硬件、flags、CPU/NUMA、seed在每次campaign的environment/manifest记录。共同规则见 [Phase2方法学](../../docs/phase2_methodology.md)。

## Baseline
CPU0、memory-node0

## Variant
固定CPU0，memory-node1；strict binding与first-touch各自对照

## Expected hardware behavior
move_pages逐页验证前后归属；bound只变memory node，first-touch默认在目标node写页。

## Run
CPU编号先与taskset/lscpu核对。下面从仓库根运行；repeat runner可用同样参数，`--variant`可隔离对照。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark numa_access --threads 1 --size 67108864 --iterations 5 --warmup 1 --cpu 0 --memory-node 1 --format json
python3 tools/run_benchmark.py --benchmark numa_access --threads 1 --size 67108864 --iterations 5 --warmup 1 --cpu 0 --memory-node 1 --repeats 3 --perf
```

## Result
本机真实三轮结果见 [Phase2报告](../../docs/phase2_results.md)，原始样本在results/raw/phase2-*；不可用能力记录NOT MEASURED，统计为null。

## perf analysis
perf_interval区间计数与run_perf全进程计数不同。只有有效的actual counters才能算作机制证据；权限拒绝保留错误，不从ns变化反推出特定miss数。

## Explanation
move_pages逐页验证前后归属；bound只变memory node，first-touch默认在目标node写页。 从单位、sample_kind、timer/setup边界解释结果；批次均值不能当单操作tail，明确哪些变量保持不变。

## When does this optimization help?
CPU/NIC/memory locality可确定且大working set触及跨node路径时。

## When might it hurt?
访问命中cache、页放置混合、自动迁页或topology不可验证时。

## Interview Questions
1. 为什么first touch要写页而非只读零页？
2. 严格mbind为什么无需在目标memory node运行CPU？
3. CPU节点与实际页节点如何独立验证？
4. 前后页归属相同为什么不能完全排除中途迁移？
5. small cache-resident workload为什么不反映DRAM远端延迟？
