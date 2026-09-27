# Next batch 候选验证记录

2026-09-20；这是实现者自查和自动化证据，不声称独立 reviewer 验收。本轮保留待验收状态。数值和环境见[实测记录](containers_results.md)。

| 维度 | 检查与处理 | 证据 |
|---|---|---|
| 网络 | 原有 UNIX socketpair 压力测试补到真实 localhost TCP；小发送缓冲区 1 MiB、short I/O/EAGAIN、半关闭后 EOF；LT/ET 连续帧与 drain 后重新到达 | `tests/test_network.cpp`；本批三种构建的 network_core |
| 容器语义 | 同样 unique key/value、查找 hit/miss、任意 key 删除、已知新 key 插入；明确序列容器与关联容器的排序/迭代器契约不同 | 六个 adapter；4000 步随机操作与独立 key-indexed oracle；全部指定 N |
| 插入分布 | 初稿插入 key 在原范围之后，可能过度有利于 sorted vector；改为区间内分散奇数 key 后再开始正式测量 | even initial keys / odd inserted keys |
| 删除分布 | 不使用原序列前缀；独立打乱后取 erase key，使位置分散 | workload 独立 shuffle |
| 生命周期 | STL 容器拥有记录；AoS/SoA vector 拥有字段；无跨线程共享；每次增删样本恢复初始状态 | ASan+UBSan，Release 下有效的 CHECK |
| 防止优化删除 | lookup/iterate 的 checksum 与命中数用于验证；insert/erase 的完整状态排序后与预先构造期望比较；校验在计时后 | 测量 body 的 compiler barrier、do_not_optimize，after callback |
| 工作量 | 每样本 ops 为查询/插入 batch、删除 min(N,batch)、遍历 N；AoS/SoA 全 pass | JSON/CSV、size=1、奇数 batch、duration 与边界 CLI tests |
| 数值 | price ticks 与 quantity 的整数算术；正式输入总和 <2^53，JSON checksum 精确；测试额外覆盖无符号模 2^64 归约 | AoS 与 SoA 同输入、零长度及非 SIMD 倍数长度测试 |
| 采样 | 热态、重复 seed 流、批均值 percentile；不把 p999 解释为逐操作尾部 | README/notes/summary 明确边界 |
| PMU | 全进程含构造、reset、校验；multiplexed IPC 使用 scaled_ipc，不能直接归因热路径 | 统一 perf runner 原始事件与 running_percent |
| 编译器 | GCC/Clang 独立 Release 构建、无 sanitizer 性能数据；检查实际 kernel 汇编 | build/test logs、两份 assembly |
| 集成 | registry、help、默认 size/batch、runner suite、CMake、说明、三轮 size campaign | 共 9 项 CTest；完整 campaign |
| 后续范围 | Order book 仅语义与三轮关卡草案，暂无 before/after；MPSC 仍为正确性证明后的可选项 | `order_book_plan.md`；不提前标完成 |

覆盖限制：未做扩容/rehash 尾部、长时间分配碎片、哈希碰撞对抗、冷缓存、多线程容器、随机记录更新的 AoS/SoA。本轮没有引入并发算法，不使用 TSan 结果作为新单线程代码性能证据。
