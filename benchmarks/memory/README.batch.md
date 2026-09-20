# Experiment: Batch object allocation

## Question
多个同时存活的64B对象使用不同分配策略会怎样？

## Hypothesis
arena可能减少元数据操作，pmr pool也可能已有很好的热缓存表现。 这是待验证假设，不是普遍结论。

## Setup
Linux、C++20、Release GCC/Clang；实际硬件、flags、CPU/NUMA、seed在每次campaign的environment/manifest记录。共同规则见 [Phase2方法学](../../docs/phase2_methodology.md)。

## Baseline
malloc、new

## Variant
arena、pmr_monotonic、pmr_pool、thread_local_arena、preallocated

## Expected hardware behavior
所有路径相同zero-init/touch；容量和生命周期限制不同；热tcache不等于生产allocator压力。

## Run
CPU编号先与taskset/lscpu核对。下面从仓库根运行；repeat runner可用同样参数，`--variant`可隔离对照。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark allocator_batch --threads 1 --batch 1024 --cpu 0 --format json
python3 tools/run_benchmark.py --benchmark allocator_batch --threads 1 --batch 1024 --cpu 0 --repeats 3 --perf
```

## Result
本机真实三轮结果见 [Phase2报告](../../docs/phase2_results.md)，原始样本在results/raw/phase2-*；不可用能力记录NOT MEASURED，统计为null。

## perf analysis
perf_interval区间计数与run_perf全进程计数不同。只有有效的actual counters才能算作机制证据；权限拒绝保留错误，不从ns变化反推出特定miss数。

## Explanation
所有路径相同zero-init/touch；容量和生命周期限制不同；热tcache不等于生产allocator压力。 从单位、sample_kind、timer/setup边界解释结果；批次均值不能当单操作tail，明确哪些变量保持不变。

## When does this optimization help?
可规划容量和统一生命周期的热路径。

## When might it hurt?
对象寿命不一致、容量超限或跨线程共用非同步资源时。

## Interview Questions
1. arena reset前为什么要结束对象生命周期？
2. alignas对象怎样在arena中保证对齐？
3. monotonic_buffer_resource为何选择null upstream？
4. unsynchronized_pool_resource可以跨线程共享吗？
5. 同时存活对象数为什么影响tcache与pool的比较？
