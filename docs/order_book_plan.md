# Order book：map/hash/list 基线与后续优化关卡

2026-09-27 按新要求推进 `map + unordered_map + list` 基线，代码、测试与测量方式见[实验说明](../benchmarks/order_book/README.md)，实测见[结果](order_book_results.md)。此文继续保留后续优化关卡；三轮优化均未执行。保持单线程、整数价格 ticks、单标的、明确容量边界；不引入 MPSC。

## 语义对照的共同契约

| 操作 | 拟定行为 | 必须比较的输出与状态 |
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

## 三轮独立改动的测量关卡

先测 baseline，定位瓶颈，记录假设，才开始下一轮修改。以下是**候选**改动，不预设收益，也不把三次重复运行当成三轮优化。

| 轮次 | before → after；仅改变的主要因素 | 验证重点 | 状态 |
|---|---|---|---|
| 1 | 相同 map/hash/list，仅预留 ID hash 容量 | 验证 rehash 是否与观测尾部相关；计时外预留与内存代价 | NOT MEASURED |
| 2 | 保留 ID 索引和 FIFO，将 map 价位目录替换为连续价位目录 | 按分布选 sorted vector 或 bounded price array；处理价位移动后的引用有效性，不同时改搜索算法 | NOT MEASURED |
| 3 | 保留价位目录、ID 索引和 FIFO 算法，仅替换订单节点分配资源为预分配可回收 pool | 相同拒绝规则；对齐、生命周期、复用；无隐藏 fallback malloc | NOT MEASURED |

每轮需 before/after 的至少三个独立进程、随机执行顺序、相同编译 flags/CPU/事件流，记录源码与二进制 hash。报告 change、why、真实均值/尾部/吞吐/内存和 perf evidence；变慢也保留，不改成“优化成功”。性能计数优先限定热路径区间；全进程 perf 若含验证和 setup，必须单独标注，不强行归因。缺少权限记 NOT MEASURED。

所有轮次都重新跑同一 semantic differential suite 及 ASan/UBSan。若并发化成为新需求，先补所有权、生命周期、happens-before 证明；MPSC 不属于此三轮。
