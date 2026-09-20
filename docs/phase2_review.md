# Phase 2 review and fixes

以Phase1交付快照为固定基线，使用code-review的Standards/Spec两个独立审查方向；规范CONTRIBUTING.md，范围phase2_scope.md。对新代码、原有框架改动与工具进行只读审查，再修复问题；没有只交付review报告。

## Standards

初审1项P2：NUMA topology查询在每variant的能力保护区之外，缺少sysfs时整个campaign会失败。已移入try，按选中variant输出NOT MEASURED并保留原因。原审查者复核通过。

没有发现可证明的UB/data race/lifetime缺陷。检查了Background的promise所有权及jthread停止/join、handoff两道barrier、TicketLock的acquire/release、Arena对齐与对象销毁、guarded mmap/smaps、perf时间差。

## Spec

初审3项P2，及同一工具的1项后续遗漏：

1. bound NUMA错误要求目标node存在允许CPU。改为bound默认在benchmark CPU first touch，绑定内存node独立；只有first_touch variants寻找目标node CPU。
2. 编译器绝对路径被用作目录tag，Path拼接可能逃出artifact目录。改为basename+resolved-path hash；正式矩阵使用 `/usr/bin/g++` 验证。
3. 单个可选IPO失败会终止整个矩阵。已独立记录可选IPO unsupported，并继续其他profile；真正compile/test失败保留FAILED并使最终返回非零。
4. 缺失objdump/ctest引发FileNotFoundError仍会中断。新增missing-executable捕获，作为tool-unavailable记录并继续独立profile。

四项均经原审查者只读复核确认修复。后续网络/数据结构/orderbook不属于本轮验收，不把它们列作遗漏。

## Local self-review fixes

- NUMA实际探测出现EINVAL，没有草率标为机器不支持：用strace与Linux 6.8 get_nodes核对，发现nodemask maxnode ABI边界；改为位容量+1。加入真实bind+move_pages回归测试，验证远端绑定成功。
- perf RESET重置counter却不重置enabled/running累计时间，已按上次读数求差，防止多轮时间重复相加。
- schema v2独立实现序列化，支持JSON控制字符、CSV引号转义、空统计与metric单位；Phase1 CSV/table合约同步更新。
- Compiler matrix flags分别控制优化级别/native/LTO，不把多个优化变化混为一组；构建、测试和测量顺序执行。

## Validation and limits

Release/ASan+UBSan/TSan套件均包含5项：Phase1 core/CLI/perf-reporting，以及Phase2结构/CLI。TSan沿用仅测试进程setarch -R，无系统级ASLR变更。日志位于evidence/phase2。

开放计划明确finite horizon与duration截断，不冒充独立网络发包器；service与scheduled response分开。THP必须前后smaps验证，NUMA必须前后每页查询，不能把policy建议当实际状态。校验快照无法排除瞬时页迁移，已文档化。

性能工具不测试错误的relaxed payload程序；正确的重复release/acquire握手在sanitizer下验证。性能相似从不构成内存模型正确性的证明。

初审：Standards 1项、Spec 3项；后续Spec补充1项，均已修复。自检另修复2项测量/Linux语义问题。实测不以审查代替，见phase2_results.md。
