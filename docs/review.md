# Phase 1 review and fixes

审查方法：使用 code-review skill，Standards 与 Spec 两个独立审查 agent；新仓库没有历史 commit，以 Git 空树为基线，需求为 docs/project_request.md，规范为 CONTRIBUTING.md。不把两个轴合成一个分数。

## Standards

初审 2 项，均为报告正确性问题：

1. **P2 表格列错位**：header 合并了 benchmark/variant、mode/sample，而 row 输出独立字段，统计标签不对应数值。已改为完全一致的 19 列，并在 CLI 测试验证所有行列数。
2. **P2 perf 实际不可计数被漏报**：probe 成功不代表组合事件运行可计数，之前只按退出码标 MEASURED。已逐项保存 actual run 的状态/原始行，区分 MEASURED / PARTIALLY MEASURED / NOT MEASURED / FAILED；模拟 not-counted 和部分计数的回归测试已通过。

独立审查未找到可证实的 UB/data race/lifetime/acquire-release 缺陷；这不等于形式化证明。没有提出必须修复的代码异味启发式项。

## Spec

初审 2 项：

1. **P2 人类可读表格错误**：与 Standards 同一问题；已修复。
2. **P2 working set 覆盖不足**：只测 batch=4096 次导致大数组顺序链仅访问小部分，却标 warm working set。已改为每样本 max(batch, size/4) 次，至少覆盖整个闭合链；改为 prefaulted 描述，不声称大数组全部 cache hot。正式容量矩阵用修复后的 binary 重测。

未将 Phase 2 功能当成 Phase 1 缺失项。

## A. C++ correctness

- SPSC 仅单 producer/consumer；head 与 tail 各自一个 writer；双方 release/acquire 保护槽位写入/读取/复用。固定 std::array 不存在热路径 malloc；有效容量 C-1，测试 full/empty/大量回绕/FIFO。
- false sharing 用 aligned operator new 与运行时 line 对齐，placement construct atomic，析构与 matching aligned delete；每 counter 原子更新且最终计数验证。
- spinlock TTAS 的 acquire/release 保护普通 counter/state；锁实验验证总更新数。pool 只接受本池指针、单线程、一次归还，测试容量耗尽和复用。
- 修复本地开发时 Block 过度对齐与 malloc 普通对齐不匹配风险；现在 Block 使用自然对齐，所有路径相同初始化。
- 工作线程的启动失败、affinity 失败在正式计时前退出；barrier 建立样本交界同步；异常不逃出线程导致 terminate。

## B. Benchmark correctness

- runtime seed/data、compiler barrier/noinline 防 DCE/hoisting；源码与 binary hashes 和 flags 存档。
- allocation 变体分派已移出计时，避免 string 比较污染分配器比较；仍包含函数调用和 timer 成本，明确承认。
- GCC Release 汇编核实 branch 中 jbe，branchless 中 cmovbe；保留 docs/evidence/branch-assembly.txt。
- timer empty loop 控制组；无机械 overhead subtraction。
- latency 与 batch_mean 清晰分列；SPSC 时间戳单独测量，不混进吞吐 pass。单次 allocation 100000 样本/轮，affinity 10000 样本/轮，3轮。
- 小样本 memory sweep 只解释均值和跨轮变化，不能宣称可靠 p999。

## C. Linux correctness

- 先验证 CPU 在 allowed mask，检查 pthread 返回码；作用域退出恢复 mask。
- 本机 core 0/1 是 node0 的不同物理核；没有设置 isolcpus、governor、boost、IRQ 或全局 ASLR。
- perf 十项事件实际被权限拒绝，全部 NOT MEASURED，IPC 为空；不自动提权。
- TSan 默认地址映射冲突保留失败日志，setarch -R 仅限 Debug 测试进程，之后完整套件通过。
- NUMA 实际 page placement、HITM、连续热状态仍 NOT MEASURED。

## D. Low-latency relevance

通过同一条学习链连接 timer → dependent memory → sharing/coherency → branch → allocation tail → synchronization → SPSC → scheduling。明确 throughput 与驻留延迟可能走向相反；不把 microbenchmark 当完整交易系统。

## E. Documentation

每项实验有统一模板、5个面试问题；README 有30秒/2分钟/深挖链接。假设不冒充结论，不将本机数据普遍化。Phase2 单列，缺失工具不生成模拟数据。

初审计数：Standards 2项，Spec 2项（共享1项）；三个独立问题已修复并由原审查者只读复核；复核还发现 README 的 batch 参数需注明 memory_access 完整遍历规则，已补充。最终验证详见 evidence 与 phase1_results.md。
