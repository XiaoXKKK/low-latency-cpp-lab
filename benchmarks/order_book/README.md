# OrderBook v0：map + unordered_map + list

## Question

简单 STL 价格时间优先撮合簿的逐事件延迟、尾部和批量吞吐是多少？将它作为保留的基线，后续每次只改变一个主要因素，并画真实 before/after。

## Hypothesis

树查找、节点分配和哈希增长可能影响热点与尾部。它们只是待验证的假设，不能凭一次延迟尖峰就认定 allocator 或 cache miss 是原因。本轮只建立 `map_list`，未进行三轮优化。

## Setup / Baseline / Variant

单线程、单标的、integer price ticks。买卖两侧分别为 `map<Price,list<Order>>`；买侧从 rbegin 取最佳价，卖侧从 begin 取最佳价；同价 list 保存 FIFO。`unordered_map<Id,Handle>` 保存稳定的 map/list iterator，以 ID 定位撤单和修改。删除最后一单时一并移除价位；完全成交时移除 ID 索引。禁止复制/移动 Book，避免复制后的 iterator 指向旧对象。

基线采用标准分配器，不提前 reserve ID hash，保留真实的节点分配、释放和 rehash。订单输出写入调用方预分配的 Trade span；没有 IO、日志或计时内日志扩容。为避免分配失败发生在部分成交之后，Add 先分配/索引订单节点再撮合；改价/增量先分配替代节点再替换。因此完全成交的 Add 也有节点分配成本。这是公开保留的基线取舍，不是最少分配实现。

## Domain semantics

- Add 先撮合对手盘，按 maker 价格成交；余量挂同侧队尾。
- Cancel 删除全部未成交量；未知 ID 返回 not_found。
- Modify 的 quantity 是新的剩余量；同价减量或等量保序，加量或改价失去原优先级并重新撮合；quantity=0 是 Cancel，此时忽略 price。side 取原订单，忽略事件的 side。
- Match 是主动方方向的市价吃单；未满足量不挂簿，成交记录的 taker ID 为 0。
- 活跃 ID 不允许重复；已完全成交或撤销的 ID 可复用。Add/Cancel/Modify 的 ID=0 非法；Add/Match 的 quantity=0 非法。
- 默认库价格范围 1..1000000，可由 Limits 缩窄。benchmark 为 1..20000。非法输入、duplicate、not_found、full、output_full 都不改变状态。
- 容量限制逻辑活跃订单数。满簿 Add 即使可以立即成交也拒绝；Modify 可在满簿替换（瞬时多一个物理节点）。所有可能撮合的操作保守要求 output.size()≥操作前活跃单数，不足先拒绝；减量/no-op/Cancel 不要求输出空间。

语义由独立 `ReferenceBook` 验证：vector 保留到达顺序，每笔成交遍历找最优候选；不共享价位遍历、ID 索引或撮合函数。测试逐事件比较全部成交、状态码、余量和完整 FIFO 簿；benchmark 在计时外比较同一事件流的 outcomes、全部成交和最终完整状态。

## Workload / measurement

`--size` 为初始挂单数（1..10000，默认 256），不是字节。既有 schema v2 的 config 仍叫 size_bytes；以 metrics.initial_orders 为准。初始买价 9500..9999、卖价 10001..10500；增单 30% 选择可跨越价差的价格范围。每 100 条标签精确含 Add/Cancel/Modify/Match=60/25/10/5，再按 seed 打乱；最后不完整 block 为确定性前缀。该比例是 synthetic，不是交易所统计。Modify 部分减量、增量和改价；Cancel/Modify 尽量选择 reference 当前活跃 ID。

**初始深度不等于稳态深度**：不额外注入未计时订单维持深度。输出初始/最低/峰值/最终订单数、实际事件数、not_found/rejected、成交数量与未满足市价量。trace_hash_hi/lo 是完整初始+事件流固定编码的 FNV-1a 64 位校验，分为两段避免 JSON double 精度损失；它不是密码学证明。命令、源码和 binary 另保存 SHA256。

| CLI | iterations | batch | warmup | 结果 |
|---|---|---|---|---|
| order_book_latency | 事件样本数，最多 100000 | 不使用 | 丢弃事件数，之后恢复初始状态 | single_event，ns/event，含 timer 开销 |
| order_book_throughput | 完整 replay 样本数 | 每 replay 事件数，最多 100000，默认 1000 | 丢弃的完整 replay 数 | replay_mean，ns/event 与 events/sec |

统一 `--variant map_list`。campaign 让 latency iterations 等于 throughput batch，保证两者相同 trace；分别做不同计时粒度的运行。latency 在 apply 前后取 steady_clock，样本存储及 offset 更新在计时外；throughput 包含事件循环、outcome journal 写入及 offset 更新。创建簿、初始填充、数据生成、恢复、校验均在计时外。duration 至少完成一个样本，latency 截断时重新计算独立 reference 前缀来校验。

latency 的 p99/p999 是该闭环 synthetic 混合事件流的逐事件分位数；不是网络端到端或开放到达排队延迟。throughput 的 p99 是整段均值分位数，不能作单事件尾延迟。样本仅 20000 时 p999 约依赖最慢 20 个事件，需看三次独立运行的离散度。

## Expected hardware behavior

map 的节点访问与 list 的 FIFO 节点访问可能形成依赖访存；hash rehash 可能造成不均匀成本；多笔 Match 的工作量本来就更大。没有用 market-data 聚合更新的指标替代撮合指标。CppCon 2024 的参考与语义区别见[阅读笔记](../../docs/order_book_cppcon_reference.md)。

## Run

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark order_book_latency --variant map_list --size 256 --iterations 20000 --warmup 1000 --cpu 0 --format json
./build/release/lab_bench --benchmark order_book_throughput --variant map_list --size 256 --batch 20000 --iterations 30 --warmup 3 --cpu 0 --format json
python3 scripts/run_order_book_campaign.py --output results/raw/order-book-v0 --cpu 0
python3 tools/plot_order_book.py --campaign results/raw/order-book-v0 --output results/processed/order-book-v0-plots
```

先检查允许 CPU；绘图 Python 环境需 `tools/requirements-plot.txt` 的 matplotlib。当前机器可使用 `/home/beihang/anaconda3/bin/python` 绘图。已有 PMU 能力时可用本机 `lab-perf` 启动 campaign；没有能力时吞吐测量仍执行，metrics.pmu_measured=0，notes 保留 PMU NOT MEASURED。

## Result / perf analysis / Explanation

本批数据、图表和验证日志见[基线结果](../../docs/order_book_results.md)。throughput 自带调用线程的 cycles/instructions perf group，每轮在重建后启用、校验前禁用；排除初始化、reference 生成、预热及内核/虚拟机计数，包含 timer/control 开销。enabled/running 使用每段增量累积；复用时不输出 cycles/event、instructions/event 或精确 IPC。失败不输出伪造零计数。

## Subsequent optimization and plots

为每版保留新的输出目录、不同 label 和同一个事件流版本，不覆盖旧记录。例如未来注册新 variant 后：

```bash
python3 scripts/run_order_book_campaign.py --variant NEW_VARIANT --label 'v1 single change' --output results/raw/order-book-v1 --cpu 0
python3 tools/plot_order_book.py --campaign results/raw/order-book-v0 --campaign results/raw/order-book-v1 --output results/processed/order-book-v0-v1
```

绘图工具支持多版叠加，检查 trace hash、编译器、flags、CPU/系统信息、warmup、样本数及语义计数；不兼容时拒绝比较。环境负载、频率和温度仍可能变化，工具检查不是因果证明。输出 summary PNG/SVG（逐事件 mean/p99/p999 和吞吐，三轮中位数及 min–max）与逐轮尾部分布图，不捏造 before/after。图和证据文件名保留版本身份。

## When help / When hurt

该基线适合学习明确语义、所有权及稳定 iterator。大量价位、分配尾部或大范围跨价位成交时，节点结构可能成为代价；真实业务的价格分布、撤改单比例和深度可能改变结论。后续连续价位目录要首先解决 iterator 失效；pool 要首先证明容量、对齐与生命周期。

## Interview Questions

1. 为什么同价减量和加量的时间优先级规则不同？
2. unordered_map rehash 会不会让已保存的 map/list iterator 失效？
3. 为什么只有 best bid/ask 和最终 checksum 一致仍不足以证明 FIFO 正确？
4. allocation failure 与输出 buffer 不足怎样避免部分处理？
5. 为什么逐事件 p99 和 replay mean p99 不能混用？
6. CppCon 的 market-data book 与此撮合簿有哪些不同？
