# Experiment: Mutex vs spinlock

## Question
相同临界区下，锁类型、竞争分组、线程数如何改变总吞吐？

## Hypothesis
短临界区且持锁者持续运行时 spin 可能有利；抢占或超额线程可能恶化。 这是待验证预期，不是最终结论。

## Setup
Linux x86-64、C++20、Release、GCC/Clang。固定 seed=42；本机硬件、compiler、flags、线程数和完整命令由 runner 的 environment/manifest/compile_commands 记录。见 [方法学](../../docs/benchmarking_methodology.md)。
竞争矩阵：--threads 4 配 --locks 4/2/1 分别为低/中/高分组争用；再分别取 --critical 0/32/256，最后单独把 --cpu 改为一个 CPU 研究 oversubscription。每次只改变一项。

## Baseline
std::mutex，shared counter 受同一把锁保护。

## Variant
TTAS spinlock：acquire test_and_set、relaxed test、pause、release clear；Phase2增加ticket lock，取号relaxed、等待serving acquire、释放release。

## Expected hardware behavior
RMW/coherency 与阻塞调度/唤醒之间权衡；不保证公平、无饥饿或低单次 acquisition latency。

## Run
在仓库根目录执行；CPU 编号需先确认属于当前 allowed mask。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark locks --threads 2 --cpu 0,1 --iterations 200 --warmup 20 --batch 16384 --locks 1 --critical 0 --format json
python3 tools/run_benchmark.py --benchmark locks --repeats 3 --perf
```

第二条用于精确参数实验，第三条使用 runner 默认参数；正式结论必须引用 manifest 中实际参数。每个变体可加 `--variant NAME` 单独运行 perf，避免把不同 workload 混为一个 counter 值。

## Result
真实数据与三轮汇总见 [Phase 1 实测](../../docs/phase1_results.md)。原始样本保存在 results/raw，统计不手工生成；未来硬件运行结果需新建目录。

## perf analysis
事件：cycles/instructions/branches/branch-misses/cache-references/cache-misses/task-clock/context-switches/cpu-migrations/page-faults。当前若权限拒绝，明确为 **NOT MEASURED**；保留错误，不把运行时间变化冒充 PMU 证据。结果文档给出本机实际状态。

## Explanation
从样本单位与计时边界开始解释，再比较同一轮的 baseline/variant。RMW/coherency 与阻塞调度/唤醒之间权衡；不保证公平、无饥饿或低单次 acquisition latency。 缺少 counters 时，只能把该机制作为推测。单次延迟与批次均值不能混用；SPSC 两类结果分开看。

## When does this optimization help?
必须测定争用和持锁时长；有独占核心预算且临界区非常短的候选场景。

## When might it hurt?
oversubscription、持锁者被抢占、长临界区或需要公平性时。

## Interview Questions
1. TTAS 为何先 relaxed test？
2. spinlock 为什么不能保证公平？
3. 持锁线程被抢占会怎样？
4. 私有锁与共享锁怎么构造低/高竞争？
5. 吞吐 ns/op 能否称作锁等待延迟？

Phase2中ticket lock体现取号顺序的代价；公平顺序不保证低tail，被排到的线程若被抢占，会阻塞后续票号。`--variant ticket`隔离运行，包含在Phase2锁矩阵。
