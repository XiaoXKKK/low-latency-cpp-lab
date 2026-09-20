# Low-Latency C++ Performance Lab

以 **假设 → baseline → 单变量修改 → 多轮测量 → profiler → 有边界的解释** 学习 C++ / CPU / Linux / HFT performance engineering。当前交付包含 **Phase 1 八个实验 + Phase 2 首批测量/Cache/Memory/同步调度/NUMA扩展**。网络、数据结构、order book 留在[后续路线图](docs/phase2.md)。[Phase 2 实测与验收](docs/phase2_results.md)记录本次进展。

## Quick start

依赖：Linux、GCC（或 Clang）、C++20、CMake ≥ 3.20、Python ≥ 3.9、pthread；perf / numactl 可选。无在线 FetchContent、无第三方 benchmark 库。

本机现已安装 Clang/LLVM/LLD 18.1.3，并为当前用户启用 perf；Clang Release 的 5 项测试和 PMU 采集均已验证。使用方式与新证据见 [工具链启用记录](docs/toolchain_setup.md)。

```bash
./scripts/build_release.sh
# 或标准 CMake 流程
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j 4
ctest --test-dir build/release --output-on-failure

./build/release/lab_bench --help
./build/release/lab_bench --benchmark timer --iterations 1000 --warmup 100 --format table
./build/release/lab_bench --benchmark false_sharing --threads 2 --cpu 0,1 --format json
./scripts/run_all.sh --cpu 0,1
```

CPU 编号只是示例，先运行 `tools/cpu_info.sh` 和 `taskset -pc $$`，选择允许集合内、拓扑明确的 CPU。本机缺少系统 CMake，已在 `.tools/` 安装官方 CMake 3.31.6，构建脚本自动使用它；该目录不纳入 Git。

## Architecture

```text
include/lab/benchmark.hpp       参数、采样、统计、编译器屏障、affinity
include/lab/structures.hpp      固定容量 SPSC、TTAS spinlock、固定块 pool
src/                           实现公共框架与实验 registry
benchmarks/{主题}/*.cpp         单变量 baseline/variant；各实验 README
 tests/                        数据结构/同步/统计测试、八项 CLI 合约
 tools/                        环境记录、随机顺序重复运行、perf 降级、Markdown 汇总
 scripts/                      Release build/test、完整运行
 docs/                         方法学、CPU/Linux、陷阱、结果、review、后续规划
 results/raw/<run>/             环境、命令、源码/二进制 SHA256、编译 flags、原始样本
 results/processed/             自动生成的 Markdown 汇总
```

实验直接链接公共库；添加实验只需新增源文件、registry 项、CMake 源列表、runner 变体表、测试和 README。保持计时、工作负载和结果解释分离；不用复杂注册宏或 UI。源代码、方法学、结果的关系见 [架构与阶段验收](docs/architecture.md)。

## Phase 1 experiments

| 实验 / CLI 名称 | baseline → variant | 采样含义 | 说明 |
|---|---|---|---|
| `timer` | 空循环 / chrono / clock_gettime / RDTSC / RDTSCP | 每批 timer read 平均 ns | [README](benchmarks/timer/README.md) |
| `memory_access` | sequential → random permutation | 依赖 load 平均 ns，非带宽 | [README](benchmarks/cache/README.memory_access.md) |
| `false_sharing` | packed → runtime line padding | 全线程聚合吞吐 | [README](benchmarks/cache/README.false_sharing.md) |
| `branch` | sorted / random × branch / branchless | 每元素批次平均 ns | [README](benchmarks/branch/README.md) |
| `allocation` | malloc/free / new/delete → pool / preallocate | 单个分配-初始化-释放延迟 | [README](benchmarks/memory/README.md) |
| `locks` | mutex → TTAS spinlock | 全线程聚合吞吐 | [README](benchmarks/synchronization/README.locks.md) |
| `spsc` | 相邻游标 → 128 B padding | 吞吐；独立的逐消息延迟测量 | [README](benchmarks/synchronization/README.spsc.md) |
| `affinity` | inherited allowed mask → pinned | 单个依赖运算任务延迟 | [README](benchmarks/scheduling/README.md) |

## Phase 2 first batch

本轮范围由用户确认：测量深化、Cache/Memory、同步调度和NUMA；共新增11个benchmark组，并扩展ticket lock。性能优化是否有效以原始数据为准。

| CLI 名称 | 对照 / 观察指标 | 说明 |
|---|---|---|
| `tsc_interval` | CPUID序列化空区间 / dependent chain；AUX过滤 | [README](benchmarks/timer/README.interval.md) |
| `perf_interval` | 区间cycles/instructions、IPC、running/enabled | [README](benchmarks/timer/README.perf_interval.md) |
| `cache_patterns` | streaming、stride、indexed、prefetch；有用bytes/sec | [README](benchmarks/cache/README.patterns.md) |
| `allocator_batch` | malloc/new/arena/PMR/thread-local/preallocated | [README](benchmarks/memory/README.batch.md) |
| `allocation_handoff` | same-thread / cross-thread free | [README](benchmarks/memory/README.handoff.md) |
| `page_behavior` | mapping、first/warm touch、THP、hugetlb验证 | [README](benchmarks/memory/README.pages.md) |
| `atomic_order` | 编译期指定的5类RMW order与3类合法store order | [README](benchmarks/synchronization/README.atomic_order.md) |
| `rw_locks` | mutex / shared_mutex，read-percent与critical | [README](benchmarks/synchronization/README.rw_locks.md) |
| `wait_strategy` | sleep/yield/spin/hybrid，lateness与CPU time | [README](benchmarks/scheduling/README.wait.md) |
| `arrival_latency` | closed/open计划、CPU/内存背景负载；response/service | [README](benchmarks/scheduling/README.arrival.md) |
| `numa_access` | strict binding / first-touch；streaming/random；逐页验证 | [README](benchmarks/numa/README.md) |

```bash
./scripts/run_phase2.sh --cpu 0,1
# 完整参考campaign；先检查本机CPU与memory node编号
python3 scripts/run_phase2_campaign.py --output results/raw/my-phase2 \
  --cpus 0,1,2,3 --memory-nodes 0,1 --compiler-matrix
# 单独编译矩阵（顺序build/test/run，不并发污染测量）
python3 tools/compiler_matrix.py --output results/raw/my-compilers --cpu 0
# 仍可单独重跑Phase1
python3 tools/run_benchmark.py --suite phase1 --cpu 0,1
```

新增参数：`--stride`（uint64元素数）、`--distance`（indexed prefetch领先访问数）、`--interval-ns`、`--read-percent`、`--memory-node`、`--touch-cpu`、`--background-cpu`。后3项默认-1，按实验规则选择。background默认和被测线程同CPU，明确构造竞争。cache/page/NUMA采用每样本完整pass，batch不控制这些样本长度。具体边界见[方法学](docs/phase2_methodology.md)。

`run_benchmark.py --benchmark all` 现在包含所有已实现实验；`--suite phase1|phase2`限定阶段；`--variant`只能搭配单个benchmark。NUMA/hugepage/perf依能力明确跳过，真正的CLI或代码失败仍使runner返回失败。

## Parameter and result contract

所有实验解析 `--iterations --warmup --threads --cpu --duration`，并支持 `--format json|csv|table`。

- `iterations`：最大测量样本数，不是内层操作数。`warmup`：每结果丢弃的预热样本数。
- `duration`：每结果测量阶段的秒数上限；与 iterations 谁先到达就停止，至少完成一个样本；不含 setup/warmup，单个批次不可抢占。SPSC 吞吐/延迟分开应用。
- `batch`：每批内层操作数（默认 4096）。memory_access 使用 `max(batch,size/4)` 保证完整覆盖；allocation 固定逐次测量；affinity 表示一个任务中的计算步数。
- `threads`：默认 1；false_sharing 要求 ≥2，SPSC 恰好 2；串行实验拒绝非 1 值，避免默默忽略。
- `cpu`：`-1`/省略继承 mask；`0,1` 按 worker 顺序循环分配。一个 CPU 加多个 worker 是有意 oversubscription，不会自动选择其他核。
- `size`：memory_access working set 字节数；`seed`：可重复数据种子；`locks`：锁分组数；`critical`：临界区依赖整数运算次数。

JSON schema v2 包含完整 config、build type、sanitizer、cache line、每条 raw samples、status/metrics/notes。CSV/表格共22列，包含相同状态与附加信息。NOT MEASURED 的统计为 null（CSV为空），不伪造零样本。多数结果 `unit=ns/op`，TSC interval 为 `tsc_ticks/op`，精确操作定义见 `sample_kind`；`mean/median/min/max/p50/p90/p95/p99/p999/stddev` 均使用该单位。标准差为总体标准差，分位数用线性插值。`ops_per_sec` 仅 throughput 有数值，latency 为 null；吞吐由总操作数/总计时时间计算。**批次均值的尾部不代表单次操作尾部**。读 TSC 的实验仍由 steady_clock 计时，未把 TSC ticks 伪称为 CPU core cycles。

```bash
python3 tools/run_benchmark.py --benchmark allocation --iterations 100000 --warmup 1000 --repeats 3 --perf
python3 tools/run_perf.py --output results/raw/perf-example -- \
  ./build/release/lab_bench --benchmark branch --variant random_branch --iterations 2000 --cpu 0
```

使用 Python runner 自动保存每次 campaign 的环境、每次命令、编译信息、哈希和原始结果；变体执行顺序按 seed 随机化。直接执行 C++ 程序只输出结果，手工实验须同时保存 `tools/environment_report.sh`。输出目录必须不存在以防覆盖。perf 不可用时保留每个事件的错误并记 **NOT MEASURED**；不自动提权或更改系统设置。

## Correctness and sanitizers

```bash
cmake -S . -B build/asan -DCMAKE_BUILD_TYPE=Debug -DLAB_SANITIZER=address
cmake --build build/asan -j 4
ctest --test-dir build/asan --output-on-failure
cmake -S . -B build/tsan -DCMAKE_BUILD_TYPE=Debug -DLAB_SANITIZER=thread
cmake --build build/tsan -j 4
ctest --test-dir build/tsan --output-on-failure
```

若 GCC TSan 报 `unexpected memory mapping`，本机已验证 `setarch x86_64 -R .tools/bin/ctest --test-dir build/tsan --output-on-failure` 可运行（仅关闭该测试进程的 ASLR）；保留原始失败日志，不用于 Release 性能测量。

address 同时启用 ASan/UBSan；thread 单独启用 TSan。CMake 拒绝 Release sanitizer。正确性测试不使用会在 Release 消失的 assert。Sanitizer 结果不用于性能结论。当前实测与工具限制见 [Phase 1 results](docs/phase1_results.md)，严格复核及修复见 [review](docs/review.md)。

## Interview Notes

| 主题 | 30 秒回答 | 2 分钟回答 | 深入追问 / 对应实验 |
|---|---|---|---|
| Timer | 先测时钟本身成本，短操作用批次计时 | 区分时钟开销、乱序、TSC tick 与 core cycle；不能直接减掉一个常数 | LFENCE/CPUID、迁核、invariant TSC；timer |
| Cache | 缓存降低局部访问成本，依赖链暴露访存延迟 | working set、prefetch、TLB、并发未命中决定结果，顺序链不是 STREAM 带宽 | 如何排除 TLB 和 NUMA？memory_access |
| False sharing | 不同变量在同一一致性单元内，写入仍可相互干扰 | 各核 RMW 引发所有权流转；padding 增大 footprint | MESI/HITM、SMT、跨 socket；false_sharing |
| Branch | 分支代价取决于可预测性和生成代码 | branchless 可能多做工作、延长依赖链；先检查汇编再测 branch-misses | 为什么随机短数组可能被学习？branch |
| Allocation | 热路径分配可能引入不稳定成本 | pool 以容量/内存换生命周期控制；tcache 已经很快，不保证 pool 获胜 | page fault、碎片、跨线程 free、p999；allocation |
| Locks | mutex/spinlock 取决于竞争和调度 | 持锁者被抢占时 spinning 浪费 CPU；TTAS 减少无效 RMW，但不保证公平 | oversubscription、优先级反转；locks |
| SPSC | 单写者和单读者可用 acquire/release 发布槽位 | 两条方向的 happens-before 防止读取未发布数据和覆盖未读数据 | 为什么不能全部 relaxed？回绕/full；spsc |
| Affinity | 固定 CPU 可减少迁移，也可能绑到忙核 | 拓扑、SMT、NUMA、负载共同影响 jitter；零观测迁移不等于不会迁移 | endpoint sampling、isolcpus、NIC locality；affinity |

先给原理，再指向源码与实测数据，最后说清 workload 边界。每个实验 README 另含 5 个面试追问。


Phase 2 Interview Notes：

| 主题 | 30秒 | 2分钟 | 深入追问 |
|---|---|---|---|
| 开放计划 | 服务快不代表响应快，还可能有积压 | 固定计划时间不会随上次完成重置；区分service与response | coordinated omission、有限horizon、随机到达 |
| PMU | cycles与TSC ticks是不同量 | 计数区间、group同步、multiplexing与权限影响解释 | enabled/running为什么不能每轮直接相加？ |
| 预取/stride | 访问更多cache line不一定得到更多有用字节 | 预取距离、index额外工作集、TLB与带宽一起决定收益 | 为什么prefetch过远可能更慢？ |
| arena/PMR | 用生命周期约束换取批量回收 | reset前结束对象寿命；PMR不自动变成线程安全 | alignment、容量耗尽、跨线程free |
| memory order | relaxed原子性不发布普通数据 | 双向release/acquire维护所有权，性能和合法性分开判断 | x86同样汇编为什么仍不能随便替换？ |
| 等待策略 | 降低lateness可能消耗更多CPU | sleep/yield/spin/hybrid按间隔与负载测量 | hybrid的spin阈值怎样选择？ |
| NUMA/hugepage | 请求policy不等于验证页状态 | 先查实际node/huge backing，再测访问模式 | first touch的读零页、写fault、临时页迁移 |
