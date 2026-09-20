# Experiment: Scheduled arrival latency and background load

## Question
闭环测量会不会掩盖积压？CPU/内存背景负载怎样改变tail？

## Hypothesis
开放计划在过载时可能积累响应延迟，即使service time变化不大。 这是待验证假设，不是普遍结论。

## Setup
Linux、C++20、Release GCC/Clang；实际硬件、flags、CPU/NUMA、seed在每次campaign的environment/manifest记录。共同规则见 [Phase2方法学](../../docs/phase2_methodology.md)。

## Baseline
closed_idle、open_idle

## Variant
open_cpu、open_memory

## Expected hardware behavior
计划到达时间独立于上次完成；保留response/service两组逐次样本。

## Run
CPU编号先与taskset/lscpu核对。下面从仓库根运行；repeat runner可用同样参数，`--variant`可隔离对照。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark arrival_latency --threads 1 --iterations 10000 --interval-ns 20000 --batch 4096 --cpu 0 --format json
python3 tools/run_benchmark.py --benchmark arrival_latency --threads 1 --iterations 10000 --interval-ns 20000 --batch 4096 --cpu 0 --repeats 3 --perf
```

## Result
本机真实三轮结果见 [Phase2报告](../../docs/phase2_results.md)，原始样本在results/raw/phase2-*；不可用能力记录NOT MEASURED，统计为null。

## perf analysis
perf_interval区间计数与run_perf全进程计数不同。只有有效的actual counters才能算作机制证据；权限拒绝保留错误，不从ns变化反推出特定miss数。

## Explanation
计划到达时间独立于上次完成；保留response/service两组逐次样本。 从单位、sample_kind、timer/setup边界解释结果；批次均值不能当单操作tail，明确哪些变量保持不变。

## When does this optimization help?
研究coordinated omission、有限到达计划及调度扰动时。

## When might it hurt?
将确定性虚拟计划推广为随机真实交易流或网络SLA时。

## Interview Questions
1. coordinated omission如何隐藏排队延迟？
2. service p99很低时response p99为什么可能很高？
3. 开放计划过载后为何不能重置下一次到达时间？
4. duration截断与有限arrival horizon如何影响解释？
5. 同CPU和不同CPU的background负载分别测试什么？
