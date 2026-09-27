# AoS / SoA：只扫描 price、quantity

## Question / Hypothesis

当记录的 id/side 不参与计算，分离字段能否降低扫描成本？假设 SoA 减少无用字段流量，并可能利于向量化；需要运行时和汇编共同验证。

## Setup / Baseline / Variant

单线程、相同 seed、整数 price ticks 与 uint32 quantity/id、char side；每次计算完整的 `sum(price*quantity)`。使用整数避免浮点 reassociation 或 fast-math 改变语义。AoS 为 `vector<Order>`，SoA 保留四个数组，但只访问 price/quantity。每个变体仅创建自己的布局；生成、分配、首次写入及 checksum 校验在计时外。

`--size` 是记录数，范围 1..1000000，默认 32768；`--batch` 不使用。正式 sweep 与容器相同，可单独扩展到 1000000。price≤1000000、quantity≤1000，总和小于 2^53，JSON checksum 可精确表示。

## Expected hardware behavior

当前 ABI 的 AoS 记录可能含 padding，实际字节数由 sizeof 输出。SoA 的访问字段为 12 bytes/record，全部逻辑存储为 17 bytes/record；不计 vector 元数据、分配器及容量碎片。输出的 useful bytes/sec 是逻辑字段吞吐，不是实测 DRAM 带宽。向量化取决于编译器、ISA 和乘法宽度，不能因为 SoA 就宣称 SIMD 已生效。

## Run

```bash
./build/release/lab_bench --benchmark data_layout --size 100000 --iterations 1000 --format json
python3 tools/run_benchmark.py --benchmark data_layout --size 100000 --repeats 3 --perf --output results/raw/layout-new
objdump -d -C build/release/CMakeFiles/lab_core.dir/src/layout.cpp.o
```

## Result / perf analysis / Explanation

见[本批进度与实测](../../docs/containers_results.md)，保存独立的 AoS/SoA kernel 汇编。编译优化作用于共同的数值算法，没有手写 SIMD。短样本计时包括函数调用，报告 ns/record full-pass mean；p99/p999 是完整 pass 均值的分位数。

## When does this optimization help?

批量访问字段子集、可做规整归约时，SoA 值得测量。

## When might it hurt?

若按单条记录频繁访问或修改所有字段，拆分数组可能增加独立寻址和维护成本；本实验没有测这些场景。小数据全部驻留 cache 时，布局流量差异不等于 DRAM 带宽差异。

## Interview Questions

1. useful bytes 与缓存行流量有什么区别？
2. 为什么检查编译器输出后才能断言向量化？
3. 结构体 padding 如何影响 AoS footprint？
4. 为什么本实验使用 integer price ticks？
5. 什么时候 AoSoA 可能值得作为后续对照？
