# Containers：相同 key/value 工作负载的六种存储方式

## Question

相同查找、插入、遍历、删除操作，连续内存与节点式容器在不同 N 下如何取舍？

## Hypothesis

连续遍历可能受益于缓存局部性；小 N 的线性查找可能抵消索引开销。sorted vector 的查找与插入/删除代价不同。这些是待测假设，不预设最快容器。

## Setup

单线程；`--size` 是初始记录数 N，范围 1..100000，默认 1024。`--batch` 是查找/插入次数，默认 64，最大 4096；删除次数为 min(N,batch)，遍历每样本完整 N 条。campaign 指定 N=16/64/256/1024/10000/100000、三轮独立进程、seed 42、轮内随机变体顺序，保存环境/编译参数/源码与二进制哈希。

初始记录为随机排列的偶数 key；value=17*key+3。lookup 使用相同随机序列，偶数 key 命中、奇数 key 未命中（奇数 batch 时多一次命中）。insert 使用已知不存在的奇数 key：batch≤N 时分散到已有 key 区间，batch>N 时延伸到区间外，然后打乱插入顺序。erase 的 key 从全体初始记录独立随机选取，不按初始容器的前缀删除。

## Baseline / Variant

`vector`、`list`、`deque` 使用线性 key 查找；`map` 使用树；`unordered_map` 使用默认哈希与默认 load factor；`sorted_vector` 使用 lower_bound。每种均有 `_lookup/_insert/_iterate/_erase` 四个变体。

此处比较的是共同的 key/value 操作，不要求相同排序或迭代器稳定性。vector/list/deque 插入已知新 key 时直接追加，map/hash 保留自身的唯一性检查。erase 包括按 key 定位，**不是已知 iterator 的 O(1) list erase**。遍历做顺序无关的 value 求和。

## Measurement

vector/sorted vector 与 hash 在 setup 中预留 N+batch 容量；插入不测 vector 扩容或 hash rehash，节点分配仍在计时内。insert/erase 每个样本前恢复初始状态，计时后将完整状态与预先生成的期望记录比较；warmup 同样执行恢复和校验。lookup/iterate 在计时外核对 checksum。状态恢复会预热内存和分配器，不称 cold cache。

每个样本重复相同查询与更新序列，可能训练分支预测器；list 的节点也可能因分配顺序而具有较好的局部性。这里不模拟长期运行的碎片化容器或每次重新随机化的查询。

结果 `ns/op` 是一批操作的平均值，p99/p999 是批均值尾部，不能称单个查找或删除的尾延迟。小 N timer/function-call 成本可能显著。`--duration` 包括测量阶段的 reset/check wall time，但不包含初始 setup/warmup；至少完成一整个样本。

## Expected hardware behavior

线性扫描可能有连续预取；链表和树依赖节点地址；哈希还受 bucket、load factor、key 分布影响。sorted vector 的移动成本随插入位置变化。不能只凭 wall time 宣称 cache miss 是唯一原因。

## Run

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark containers --size 1024 --batch 64 --format json
./build/release/lab_bench --benchmark containers --variant list_erase --size 10000 --iterations 100
python3 scripts/run_container_campaign.py --cpu 0 --output results/raw/containers-new --perf
```

先确认允许 CPU。仅当本机已有 perf 权限时使用 perf；统一 runner 在权限不足时保留 NOT MEASURED，不更改全局配置。

## Result / perf analysis / Explanation

见[本批进度与实测](../../docs/containers_results.md)。perf 为独立运行的全进程计数，包含 setup、warmup、reset 和完整状态校验；特别是 insert/erase 的排序校验可能支配 PMU。不能用这些计数除以样本操作数，或作为热路径缓存归因的证据。三轮 batch 实测也不足以验证单操作 p99.9。

## When does this optimization help?

连续存储适合密集遍历；索引可能适合大量随机 key 查找。应将本实验与实际读写比例、排序及引用稳定性要求一起判断。

## When might it hurt?

vector 的移动、hash 的额外容量、list 的节点分配都可能不适合特定业务；此处预留容量结果不覆盖扩容尾部、恶意哈希冲突和长时间内存碎片。

## Interview Questions

1. 为什么 list 的 O(1) erase 不包括按 key 寻找节点？
2. 为什么小 N 的 vector 查找可能有竞争力？
3. sorted vector 的查找收益为什么不能推导出插入收益？
4. reserve 与 rehash 改变了哪些计时边界？
5. checksum、完整状态校验与防止 dead-code elimination 各有什么作用？
