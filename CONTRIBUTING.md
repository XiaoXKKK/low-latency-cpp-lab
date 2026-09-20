# 实验贡献规则

- C++20；开启 Wall/Wextra/Wpedantic。Release 结果禁止 sanitizer。
- 并发代码必须先说明所有权、生命周期和 happens-before；没有证明的 relaxed 发布禁止合入。
- 一次比较改变一个主要变量；给 baseline、可重复 seed、原始样本和环境。
- 明确 latency / throughput、采样单位和计时边界；批次均值的 p99 不能称为单次 p99。
- setup、分配、绑核、线程创建尽可能离开计时区间；无法排除的成本写入 notes。
- 不伪造结果；不可用的工具、事件、硬件标为 NOT MEASURED。
- 新实验需 README（Question/Hypothesis/Setup/Baseline/Variant/Expected hardware behavior/Run/Result/perf analysis/Explanation/When help/When hurt/Interview Questions）、CLI smoke、必要结构正确性测试。
- 不把一个 workload 的结果泛化成普遍结论；先 Measure，再假设，再单变量修改，再复测。
- 不擅自修改系统 governor、perf 权限、隔离核或全局调优。
