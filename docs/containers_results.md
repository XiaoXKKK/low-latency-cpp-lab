# Next batch：容器 / AoS–SoA 候选进度与本机实测

2026-09-20。本轮按要求保持 **待验收**，不将 Networking → containers/AoS–SoA → order book 整条路线标为已实现。以下区分可运行代码、实际测试与实测、尚未推进的工作。

## 当前范围

- 原有 networking 代码通过回归；新增真实 IPv4 localhost TCP 的 1 MiB 小发送缓冲区测试、LT/ET 连续帧和耗尽后再到达。已有 RTT/UDP/busy-loop 实验继续保留，本次没有重跑完整网络性能 campaign。
- 容器候选：vector/list/deque/map/unordered_map/sorted vector，各含 lookup/insert/iterate/erase，合计 24 变体。
- AoS/SoA 候选：相同 seed、相同整数 `price*quantity` 归约，两个变体；汇编已核实，而非按结构名称推测 SIMD。
- Order book：只有[语义与三轮独立优化草案](order_book_plan.md)，没有实现、before/after 数据或“优化完成”结论。MPSC 保持有正确性证明后再推进的可选项目。

详细边界见[容器说明](../benchmarks/containers/README.md)、[布局说明](../benchmarks/containers/README.layout.md)，自查见[验证记录](containers_review.md)。

## 构建与正确性

| 配置 | 结果 | 原始日志 |
|---|---|---|
| GCC 13.3 Release，-O3 -DNDEBUG，无 native/LTO/sanitizer | 9/9 PASS | [Release](evidence/containers/release-tests.log) |
| GCC Debug，ASan+UBSan | 9/9 PASS | [sanitizers](evidence/containers/asan-tests.log) |
| Clang 18.1.3 Release，-O3 -DNDEBUG，无 native/LTO/sanitizer | 9/9 PASS | [Clang](evidence/containers/clang-tests.log) |

新增结构测试使用独立 key-indexed oracle 做每容器 4000 步随机操作；覆盖 hit/miss、成功/未知/重复 erase、clear/reset、全部指定 N，AoS/SoA 还覆盖空输入、非 SIMD 整数倍长度、无符号算术。CLI 测试验证 26 个变体、ops/sample、checksum、JSON/CSV、duration、size/batch/thread/variant 边界。没有以“某实现必须更快”为测试条件。

## 测量方法

机器：AMD EPYC 7C13，2 sockets，256 logical CPUs，SMT 开启；Linux 6.8.0-136-generic。主 sweep 使用 GCC Release，单线程固定 CPU 0，seed 42；N=16/64/256/1024/10000/100000；每轮每变体 warmup=10、measured samples=100、batch=64，三个独立进程，轮内随机变体顺序。构建、测试结束后才启动测量，没有并发编译或性能任务。不更改系统 governor、隔离核或 perf 权限。

原始目录：`results/raw/containers-20260920/`，包含每轮原始样本、命令、环境、源码/二进制 SHA256、编译配置。执行日志见[campaign log](evidence/containers/campaign-run.log)。下表每格为三个进程的 **mean 的中位数**，单位 ns/operation（遍历/布局为 ns/record）。100 个 batch 样本只能作初步性能比较，不能验证单次操作 p99.9。

### Lookup：重复查询的热态工作负载

| N | vector | list | deque | map | unordered_map | sorted_vector |
|---:|---:|---:|---:|---:|---:|---:|
| 16 | 12.399 | 7.749 | 13.011 | 10.332 | 2.802 | 4.829 |
| 64 | 24.974 | 146.307 | 45.952 | 6.802 | 5.670 | 14.072 |
| 256 | 58.489 | 297.663 | 80.193 | 17.992 | 7.395 | 17.861 |
| 1024 | 264.729 | 1030.291 | 354.998 | 23.140 | 3.713 | 14.438 |
| 10000 | 1847.377 | 10412.799 | 2913.414 | 18.290 | 3.990 | 17.709 |
| 100000 | 23438.627 | 106341.423 | 50043.871 | 32.634 | 3.906 | 21.903 |

每个样本重复同一批 64 个查询；索引容器涉及的热 key 子集很小。该表不能当作对整个 N 的全新随机访问性能。小 N 时 timer、频率和调度噪声比例很大，三轮数据不是可靠的普适排名。

### N=100000 的插入与删除

| 容器 | insert ns/op | erase ns/op |
|---|---:|---:|
| vector | 1.971 | 34245.380 |
| list | 12.691 | 154092.345 |
| deque | 5.803 | 57155.560 |
| map | 228.712 | 235.578 |
| unordered_map | 46.495 | 74.582 |
| sorted_vector | 35319.637 | 32111.950 |

insert 预留容量，序列容器追加已知新 key；sorted vector 在 key 区间内插入。erase 包含定位，全部完成同一批 key 的删除。这里的快慢包含不同容器维护自身排序/索引的成本。

### 完整遍历与布局归约

| N | vector iterate | list iterate | AoS | SoA |
|---:|---:|---:|---:|---:|
| 16 | 4.421 | 2.993 | 4.959 | 2.386 |
| 64 | 1.355 | 1.919 | 1.554 | 2.028 |
| 256 | 0.347 | 3.506 | 0.847 | 0.604 |
| 1024 | 0.557 | 1.724 | 0.438 | 1.043 |
| 10000 | 0.247 | 1.646 | 0.376 | 0.473 |
| 100000 | 0.307 | 1.668 | 0.464 | 0.494 |

vector/list 为 value 求和，AoS/SoA 为 price×quantity 归约；只能在各自一对内部比较。N=100000 的 vector 遍历约为 list 的 5.4 倍吞吐，但 N=16 的结果不能支持“vector 总是更快”。GCC 下 N=10000/100000 时 SoA 已生成 SIMD，耗时仍略高于 AoS。

### 布局补测：编译器和工作集边界

仍为 CPU 0、每轮 100 pass、warmup 10、三轮独立进程；补测在主 sweep 后串行运行。

| Compiler | N | AoS ns/record | SoA ns/record |
|---|---:|---:|---:|
| GCC | 100000 | 0.464 | 0.494 |
| Clang | 100000 | 0.488 | 0.342 |
| GCC | 1000000 | 0.585 | 0.491 |
| Clang | 1000000 | 0.604 | 0.349 |

N=1000000 时 AoS 逻辑存储为 24 MB，SoA 为 17 MB（访问字段 12 MB）。SoA 在两种编译器下都较快，但不能据此把所有差异归因于带宽。GCC 与 Clang 的 SoA 汇编形状不同；没有隔离指令选择、展开与内存影响的额外实验，不宣称已证明唯一瓶颈。

完整 24 个容器变体和布局的 p50/p99/p999、吞吐见每档原始汇总：

- [containers-n16](evidence/containers/containers-n16-summary.md)
- [containers-n64](evidence/containers/containers-n64-summary.md)
- [containers-n256](evidence/containers/containers-n256-summary.md)
- [containers-n1024](evidence/containers/containers-n1024-summary.md)
- [containers-n10000](evidence/containers/containers-n10000-summary.md)
- [containers-n100000](evidence/containers/containers-n100000-summary.md)
- [layout-clang-n100000](evidence/containers/layout-clang-n100000-summary.md)
- [layout-clang-n1000000](evidence/containers/layout-clang-n1000000-summary.md)
- [layout-gcc-n1000000](evidence/containers/layout-gcc-n1000000-summary.md)

主 sweep 共 468 个独立变体进程；布局补测 18 个，共 486 份结果。26 组全进程 perf 全部采到数值，cycles/instructions 的 running coverage 为 70%–98%，均只报告 scaled IPC。参见 [PMU 原始报告集合](evidence/containers/perf-summary.json) 与 [命令和哈希](evidence/containers/provenance.json)。

## 编译器证据与解释范围

GCC/Clang 的两份当前 object 反汇编分别见 [GCC assembly](evidence/containers/gcc-layout-assembly.txt)、[Clang assembly](evidence/containers/clang-layout-assembly.txt)。GCC 的 AoS 使用标量 `imul`、每次记录指针前进 0x18；Clang AoS 使用两次标量乘法的展开循环。两者 SoA 都生成 `pmuludq`、`paddq` 和标量余数路径，属于 SSE 向量化。这里没有启用 -march=native，也没有手写 SIMD。

布局只改变一次主要源代码因素；编译器因此改变循环形状是测量的一部分。这个对照没有把“减少无用字段流量”和“向量化”两种机制进一步隔离，不能只按 wall time 确认 cache/DRAM 瓶颈。

N=1024 的 perf 为独立全进程采集，包含 setup、warmup、reset 和完整状态验证。增删的排序验证可能支配计数，不把 PMU counts 除以 timed operations 当作热路径指令数。计数器复用时报告 `scaled_ipc` 和 running coverage；权限/事件不足则保留 NOT MEASURED。完整事件记录保留在原始目录。

容器结论只适用于这组整数 key、预留容量、重复查询、热态分配器。不同容器的排序和迭代器语义并不相同。布局的 useful bytes/sec 只计算 price/quantity 字段，不等于 DRAM 带宽；所有 p99/p999 都是 batch/pass 平均值的分位数。

## 下一步关卡

维持本批待验收状态；Order book 先实现独立 reference 和朴素 baseline，锁定逐事件语义，再执行三轮单因素 before/after/perf 对照。[计划](order_book_plan.md)明确记录目前三轮均为 NOT MEASURED。不能把本批的三次重复测量算作 order book 的三轮优化。
