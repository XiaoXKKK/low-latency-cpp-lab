# Experiment: Cross-thread free

## Question
同一producer分配的对象改由另一个线程释放会怎样？

## Hypothesis
跨线程释放可能改变allocator caches和coherency成本。 这是待验证假设，不是普遍结论。

## Setup
Linux、C++20、Release GCC/Clang；实际硬件、flags、CPU/NUMA、seed在每次campaign的environment/manifest记录。共同规则见 [Phase2方法学](../../docs/phase2_methodology.md)。

## Baseline
same_thread_free

## Variant
cross_thread_free

## Expected hardware behavior
两个版本保留相同barriers；仅改变释放者；publish/reuse各由一次barrier保证。

## Run
CPU编号先与taskset/lscpu核对。下面从仓库根运行；repeat runner可用同样参数，`--variant`可隔离对照。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark allocation_handoff --threads 2 --batch 1024 --cpu 0,1 --format json
python3 tools/run_benchmark.py --benchmark allocation_handoff --threads 2 --batch 1024 --cpu 0,1 --repeats 3 --perf
```

## Result
本机真实三轮结果见 [Phase2报告](../../docs/phase2_results.md)，原始样本在results/raw/phase2-*；不可用能力记录NOT MEASURED，统计为null。

## perf analysis
perf_interval区间计数与run_perf全进程计数不同。只有有效的actual counters才能算作机制证据；权限拒绝保留错误，不从ns变化反推出特定miss数。

## Explanation
两个版本保留相同barriers；仅改变释放者；publish/reuse各由一次barrier保证。 从单位、sample_kind、timer/setup边界解释结果；批次均值不能当单操作tail，明确哪些变量保持不变。

## When does this optimization help?
了解生产者分配、消费者回收的消息生命周期时。

## When might it hurt?
误将barrier-inclusive throughput当成单次free tail时。

## Interview Questions
1. 分配线程与释放线程不同会怎样影响allocator caches？
2. barrier如何发布普通pointer数组？
3. 为什么第二次barrier是复用前的必要条件？
4. cross-thread free吞吐能否代表free调用的单次tail？
5. 用消息队列移交对象时谁拥有析构责任？
