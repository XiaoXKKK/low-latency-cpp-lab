# Order book：map/hash/list 基线与后续优化关卡

2026-09-27 按新要求推进 `map + unordered_map + list` 基线，代码、测试与测量方式见[实验说明](../benchmarks/order_book/README.md)，实测见[结果](order_book_results.md)。此文保留优化关卡；本轮实现和验证进度见[逐轮报告](order_book_cppcon_results.md)，性能状态以报告中的实测证据为准。保持单线程、整数价格 ticks、单标的、明确容量边界；不引入 MPSC。

## 语义对照的共同契约

| 操作 | 行为 | 必须比较的输出与状态 |
|---|---|---|
| Add(id,side,price,qty) | 新增限价单；先按价格时间优先撮合对手盘，剩余量挂在己方价位队尾 | 状态码、逐笔成交 maker/taker ID、maker 价格、数量、剩余量 |
| Cancel(id) | 删除未成交余量；未知 ID 返回 not_found | 删除状态、价位 FIFO、活跃订单数 |
| Modify(id,price,qty) | qty 表示新的剩余量；同价减量保留优先级；同价加量或改价取消旧优先级，按新 Add 重新撮合/排队；qty=0 等同 Cancel | 逐笔成交与全部价位 FIFO；同价等量为 no-op |
| Match(side,qty) | side 为主动成交方；按最佳价格、同价 FIFO 吃对手盘，余量不挂单 | 逐笔成交、未满足数量、清空价位和 best bid/ask |

价格与数量使用固定宽度无符号整数；price 必须在共同的已知闭区间内。ID=0、重复活跃 ID、非法价格/side、Add qty=0、容量不足均在修改状态前拒绝。已完全成交或撤销的 ID 允许复用；测试必须防止旧索引指向复用槽位。Modify 保留 side，原 ID 保持业务身份。输出 buffer 容量在事件前检查，禁止部分撮合后才因日志扩容失败而留下不可解释状态。

这些是实验自定规则，不声称符合某个真实交易所。容量约束和输出策略必须在 baseline 与所有 variants 中一致；价格范围优化不得悄悄改变可接受的输入。

## Baseline 与独立 reference

可读 baseline：两侧 `map<price, list<Order>>`，价位内 list 维护 FIFO，`unordered_map<ID,Handle>` 保存稳定的价位与订单 iterator。使用默认分配器，不预留 hash 容量。用户这次明确指定 ID hash 从基线开始存在，因此不能再将“增加 ID 索引”算作后续第一轮优化。分配失败保护需要先分配待撮合节点，这一成本保留在基线中。

另写朴素 vector/reference：逐次扫描候选订单，按 `(price priority, arrival sequence)` 选择下一笔成交，不复用 baseline 的价位遍历或索引代码。测试逐事件比较返回码、**完整成交序列**、每价位完整 FIFO、逐单剩余量、best prices、总量；只比最终 checksum 不足以证明语义相同。

确定性用例覆盖：双向跨多个价位、同价 FIFO、部分成交、空簿、最后一单删除、减量保序/加量失序/改价立即成交、重复 ID、未知撤单、ID 复用、价格边界、容量耗尽、拒绝操作不改变状态。随机 replay 使用多 seed、非法输入及长序列，每一步与 reference 比较；Release 的断言不依赖 `assert`。

## 工作负载与计时

- synthetic 60% Add、25% Cancel、10% Modify、5% Match；按每 100 个事件精确构成后按 seed 打乱。比例只是教学输入，不声称真实交易所流量。
- 在 setup 用 reference 生成事件和目标 ID，同时记录实际成功、not_found、rejected、成交数；操作标签比例不能替代实际执行比例。
- 预生成事件，setup/warmup 后重放同一条测量流，各实现初始状态相同。记录价格范围、活跃单数、深度、seed、事件序列哈希。容量不足也必须统一处理并报告，不静默丢弃事件。
- 单事件 steady_clock bracket 产生 mean/p50/p99/p999（含 timer 成本）；另用相同事件流做批量 throughput，避免用 batch p99 充当单事件尾延迟。说明 closed-loop 与计划到达延迟的区别。
- 同一个事件流、同一个输出 sink，各变体相同校验边界。内存分配是否在计时内需要逐版本记录。

## 按 CppCon 顺序推进的测量与优化关卡

基线 commit：`e82ed12`（包含前序容器实验依赖和 v0 原始证据）。按用户新的图形与顺序要求，主线遵循 CppCon 2024 讲义顺序。讲义是行情价位聚合簿，本项目仍保留撮合/FIFO 语义；每一步需要自己的正确性和测量证据。先前提出的 hash reserve、bounded array 和 pool 改为可选支线，不插入以下主线。

| 阶段 / 讲义印刷页 | before → after；仅改变的主要因素 | 验证重点 | 状态 |
|---|---|---|---|
| 测量前置，30–31 | 相同 map/hash/list、相同逻辑事件流，顺序槽位 → 随机槽位布局（default map 另留对照） | 截图启发此对照；只改变物理分配布局，不打乱事件/FIFO；独立 allocation seed；不能算加速优化 | MEASURED，见[逐轮报告](order_book_cppcon_results.md) |
| 优化 1，33–37 | map 价位目录 → 有序 vector + lower_bound | 保留订单 list/FIFO、拒绝和撮合语义；解决 vector 移动后的 ID→价位关联有效性 | MEASURED，见[逐轮报告](order_book_cppcon_results.md) |
| 优化 2，38–41 | 同样 vector，反转价位排序，把最佳价位放末端 | 先记录更新/插入位置分布；只调整存储方向，观察搬移数量 | MEASURED，见[逐轮报告](order_book_cppcon_results.md) |
| 分析前置，44–48 | 使用热点区间 perf/top-down/调用栈定位 | 排除初始化及 oracle；记录可用计数与边界 | 热 replay PMU 与汇编已测；top-down/调用栈未测 |
| 优化 3，49–53 | lower_bound → branchless 二分 | 价位布局和语义不变；匹配命中/未命中、边界、汇编与计数器 | MEASURED，见[逐轮报告](order_book_cppcon_results.md) |
| 优化 4，54 | branchless 二分 → 线性搜索 | 固定相同布局与查找方向；依据价位访问分布测试，保留变慢结果 | MEASURED，见[逐轮报告](order_book_cppcon_results.md) |

讲义后面的 likely/unlikely（57–58）、cold/noinline 错误路径（59–61）、lambda/functor vs std::function（62）仅在本项目出现对应热点后再考虑。基线没有 std::function 撮合回调，不人为加一个再删掉制造“优化”。官方来源与页码见[参考笔记](order_book_cppcon_reference.md)。

随机化分配的具体实现未在讲义中给出。本轮采用仅影响价位 map 节点的固定槽位资源、setup 排列/预触碰、独立 seed=1729；顺序与随机控制采用相同分配/复用机制，并验证实际相对地址。详情见[逐轮报告](order_book_cppcon_results.md)。不能简单换 event seed 或打乱输入订单，因为那会改变 FIFO、成交及访问分布。保留相同 trace hash；分配布局 seed 作为独立配置记录。参考截图中的 33/63 ns 不作为本机结果或目标承诺。

## 每步的图与留档

主图采用同 workload、同独立轮次的**重叠频数直方图**：横轴单事件 latency(ns)，纵轴真实样本数，共享 bin 边界，半透明填充，每版用同色虚线标出全部样本的 median。默认展示第 0 轮；其他轮分别画，不能择优选一轮。需要合并显示时显式选择 all，标明 pooled，不将其解释为更多独立实验。

显示窗口与统计范围分开：例如裁到 500 ns，只限制图的横轴；中位数/尾部统计仍使用全部样本，图下注明超出范围的数量、比例及 max。保留完整 CCDF、三轮 min–max 概览和原始样本。比较频数时样本量必须相等；不同样本量仅允许显式的全样本概率归一化。未经测量的版本不画第二条分布。

每轮需 before/after 的至少三个独立进程、随机执行顺序、相同编译 flags/CPU/事件流，记录源码与二进制 hash。报告 change、why、真实均值/尾部/吞吐/内存和 perf evidence；变慢也保留，不改成“优化成功”。性能计数优先限定热路径区间；全进程 perf 若含验证和 setup，必须单独标注，不强行归因。缺少权限记 NOT MEASURED。

所有轮次都重新跑同一 semantic differential suite 及 ASan/UBSan。若并发化成为新需求，先补所有权、生命周期、happens-before 证明；MPSC 不属于此优化主线。
