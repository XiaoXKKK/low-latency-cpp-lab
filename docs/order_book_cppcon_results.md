# OrderBook：按 CppCon 顺序进行的对照与优化

2026-09-27。历史基线 `e82ed12`、直方图提交 `7fcbcc8` 保留。本轮继续原有撮合/FIFO 语义，不把讲义的行情聚合簿性能当作本机目标。依据与页码见[参考笔记](order_book_cppcon_reference.md)。

## 实现顺序与边界

| 阶段 | before → after | 唯一主要变量与必要适配 |
|---|---|---|
| 分配布局对照 | map_slots_ordered → map_slots_random | 同一固定槽位资源，初始空闲槽位顺序 → seed=1729 随机排列；仅价位 map 节点受影响 |
| 1：vector | map_list → vector_front | map 目录 → vector + lower_bound，最佳价在前；ID index 改存稳定 list iterator，删除时按 price 重找目录 |
| 2：反转 | vector_front → vector_back | 最佳价从前端移至末端；保留 lower_bound 和全部撮合逻辑 |
| 3：branchless | vector_back → vector_branchless | 保持目录方向，换成固定迭代次数的二分搜索；比较结果用条件选择推进 base |
| 4：linear | vector_branchless → vector_linear | 保持目录方向，从最佳价末端开始线性搜索，返回相同 lower_bound 插入位置 |

布局实验不是加速轮次。`map_list` 仍使用默认分配器；槽位两组都在 setup 分配并触碰内存，初始槽位 64 字节对齐，步长按实际 map 节点大小向上取整。资源容量为逻辑 max_orders+1，允许 Modify 临时替代节点。复用均采用相同 LIFO 栈，随机版不在热路径调用 PRNG。初始排列 seed 与事件 seed=42 分开，地址排列哈希与相邻地址平均距离另存。它验证明确的物理布局假设，**不声称复刻讲义没有公布的 randomized allocator**，也不能把默认 malloc 与槽位版的差异单独归因于布局。

vector 直接保存 `(Price, list<Order>)`，没有额外价位对象分配。目录搬移使用 noexcept list move；标准 allocator 相等，list 节点迭代器仍有效。ID 索引不保存 vector iterator、指针或下标；因此失去 map 稳定价位 iterator 所省下的查找。这是换容器所必需的适配成本，必须计入比较。各 vector 版本均不预留 vector/hash 容量，不添加 pool，Add/Modify 仍先准备可能分配的节点再撮合。

## 验证

- GCC Release、Clang Release、GCC ASan+UBSan 各 **13/13 PASS**。七个版本都跑相同的完整状态/FIFO 差分测试，每版 32,000 个随机事件；另覆盖 0..65 个价位的全部插入间隙、双向改价、整簿吃单和 2,000 单 hash 增长/撤单。
- 分配失败注入对默认 map 与四个 vector 版本逐一失败 Add/Modify 的每个分配点；异常后完整逻辑状态不变。容量增长本身可以保留，不属于交易逻辑状态。
- 槽位测试验证真实相对地址排列可复现、不同 seed 改变排列、顺序布局步长、唯一地址、耗尽和释放复用。
- 两个编译器均无编译告警。测试及 build 日志见 [evidence](evidence/order-book-cppcon/)。

## 测量设计

同一最终 Release binary 重新测量全部七版。每种初始深度 16/256/4096 × latency/throughput 各三个独立进程；每一轮随机排序七版运行，顺序记录在 interleaved manifest，不并行测量。latency 每轮 20,000 事件、warmup=1000；throughput 每轮 30 次 × 20,000 事件、warmup=3。相同 event seed、trace hash、CPU、编译 flags、输出日志和 oracle。历史 v0 只留档，不混入本轮 before/after。

PMU group 在每段热 replay 前开启、结束立即关闭；包含 cycles、instructions、branches、branch misses、通用 cache misses。reset、reference、验证与结构统计均在区间外。计数不可用或发生复用时不输出精确 per-event 归因。这里的通用 cache-misses 不是 L1 miss，更不是完整 top-down 分解；本轮不冒充获得 backend-bound 比例。

结构统计使用单独的 `lab_order_book_diagnostics`，计数逻辑在 timed variants 中编译消除。它记录目录插入/删除次数和搬移的 Level 数，不等同于 CPU 字节流量；不会统计 vector 扩容额外搬移。查询 rank 桶按返回的插入位置相对最佳端定义，包含未命中，因此 front/back 的 rank 桶不要求完全相同。目录 capacity bytes 不包含 list/hash 节点，槽位 bytes 不代表整进程内存。

GCC Release 的 branchless `ensure` 和 `unlink` 中已观察到 `cmov`，保存[汇编](evidence/order-book-cppcon/branchless-assembly.txt)；循环、空目录和 side 选择仍有分支，不能说整个 OrderBook 无分支。热点区间分支计数用于判断算法改变是否影响失误，不只依靠源代码写法。

## 复现

```bash
./scripts/build_release.sh
# 在本机已有 perf capability 的 launcher 下运行；普通环境可直接 python3，缺少 PMU 会明确记录。
lab-perf python3 scripts/run_order_book_sequence.py --cpu 0 --output results/raw/order-book-cppcon-20260927
python3 tools/plot_order_book.py --campaign results/raw/order-book-cppcon-20260927/vector_front --campaign results/raw/order-book-cppcon-20260927/vector_back --output results/processed/order-book-round2
./build/release/lab_order_book_diagnostics
```

延迟图默认显示固定第 0 轮，共享 5 ns 分箱、完整样本 median 和窗口外 tail 计数；完整 CCDF 展示三轮，概览图展示三轮中位数与 min–max。所有原始事件样本均留档。三次进程重复不是三轮优化；上述四轮各有独立 before/after 定义。

## 状态

实现与验证已完成，性能 campaign 待运行；不得从结构变化或汇编推断已经加速。
