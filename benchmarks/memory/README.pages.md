# Experiment: Mapping, faults and huge pages

## Question
首次触页与热页、普通页与实际hugepage backing有何差异？

## Hypothesis
首次写可能触发fault；THP可能改变TLB覆盖，也可能带来分配/整理成本。 这是待验证假设，不是普遍结论。

## Setup
Linux、C++20、Release GCC/Clang；实际硬件、flags、CPU/NUMA、seed在每次campaign的environment/manifest记录。共同规则见 [Phase2方法学](../../docs/phase2_methodology.md)。

## Baseline
mapping_roundtrip、first_touch、warm_touch

## Variant
thp、hugetlb

## Expected hardware behavior
mapping和touch分开；huge backing前后验证，不只看madvise返回值。

## Run
CPU编号先与taskset/lscpu核对。下面从仓库根运行；repeat runner可用同样参数，`--variant`可隔离对照。

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark page_behavior --threads 1 --size 2097152 --cpu 0 --format json
python3 tools/run_benchmark.py --benchmark page_behavior --threads 1 --size 2097152 --cpu 0 --repeats 3 --perf
```

## Result
本机真实三轮结果见 [Phase2报告](../../docs/phase2_results.md)，原始样本在results/raw/phase2-*；不可用能力记录NOT MEASURED，统计为null。

## perf analysis
perf_interval区间计数与run_perf全进程计数不同。只有有效的actual counters才能算作机制证据；权限拒绝保留错误，不从ns变化反推出特定miss数。

## Explanation
mapping和touch分开；huge backing前后验证，不只看madvise返回值。 从单位、sample_kind、timer/setup边界解释结果；批次均值不能当单操作tail，明确哪些变量保持不变。

## When does this optimization help?
大工作集、TLB压力与可验证的huge backing同时存在时。

## When might it hurt?
hugepage分配失败、内存碎片、collapse成本或footprint增大时。

## Interview Questions
1. 首次read匿名页与首次write为何不同？
2. madvise成功是否证明得到了THP？
3. AnonHugePages为何必须限定到被测VMA？
4. THP collapse成本与hot touch成本应该如何分开？
5. 显式hugetlb池耗尽与THP失败有什么区别？
