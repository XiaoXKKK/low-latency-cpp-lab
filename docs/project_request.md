你需要为我构建一个完整的、可长期扩展的 **Low-Latency C++ Performance Lab**。

项目目标：

1. 系统学习并验证 Low-Latency C++ / Quant Developer / HFT Software Engineer 常见性能优化知识。
2. 每个性能结论必须尽可能通过实验验证，而不是停留在理论描述。
3. 项目既要适合秋招/面试复习，也要适合作为真实的 performance engineering playground。
4. 最终形成一个可以持续添加实验的 benchmark repository。

项目暂命名：

`low-latency-cpp-lab`

主要运行环境默认：

* Linux x86-64
* GCC / Clang
* C++20
* CMake
* Release build
* 单机多核 CPU
* 支持 perf
* 支持 taskset
* 支持 numactl
* 如果机器支持，则使用 RDTSC / RDTSCP
* 如果机器支持 NUMA，则增加 NUMA 实验
* 不要求依赖专用 HFT 网卡

不要一开始追求复杂 UI。

重点是：

* 实验设计
* benchmark correctness
* measurement methodology
* reproducibility
* performance analysis
* runnable code
* technical documentation

---

## 一、核心原则

这个项目的最重要原则：

**任何性能优化结论都不能只根据经验或理论直接下结论。**

对于可以测试的结论，必须：

1. 建立 baseline。
2. 修改一个主要变量。
3. benchmark before / after。
4. 至少重复多轮测试。
5. 输出统计指标。
6. 尽可能使用 perf 或其他 profiler 验证原因。
7. 说明测试环境。
8. 解释实验可能存在的偏差。

禁止：

* “理论上更快，所以一定更快”
* “lock-free 一定比 mutex 快”
* “spinlock 一定适合低延迟”
* “cache friendly 一定性能更高”
* “branchless 一定更快”
* “io_uring 一定比 epoll 更快”
* “hugepage 一定提升性能”

这些结论必须通过具体 workload 和数据讨论。

所有 benchmark 必须避免明显的 compiler optimization 污染，例如：

* dead code elimination
* constant folding
* loop hoisting
* unused result elimination

必要时使用：

* benchmark barrier
* DoNotOptimize
* volatile
* asm barrier

但不要滥用 volatile。

---

## 二、项目目录设计

请设计类似：

low-latency-cpp-lab/

README.md

docs/
benchmarking_methodology.md
cpu_architecture.md
linux_tuning.md
measurement_pitfalls.md

benchmarks/

```
timer/
    chrono_cost.cpp
    rdtsc_cost.cpp
    rdtscp_cost.cpp
    clock_gettime_cost.cpp

cache/
    sequential_vs_random.cpp
    cache_line.cpp
    false_sharing.cpp
    cache_capacity.cpp
    cache_stride.cpp
    prefetch.cpp

memory/
    malloc_vs_pool.cpp
    stack_vs_heap.cpp
    arena_allocator.cpp
    pmr.cpp
    hugepage.cpp
    mmap.cpp
    page_fault.cpp

branch/
    predictable_branch.cpp
    unpredictable_branch.cpp
    branchless.cpp
    lookup_table.cpp

synchronization/
    mutex.cpp
    spinlock.cpp
    shared_mutex.cpp
    atomic.cpp
    atomic_memory_order.cpp
    spsc_ring_buffer.cpp
    mpsc_queue.cpp
    false_sharing_atomic.cpp

scheduling/
    thread_affinity.cpp
    cpu_migration.cpp
    context_switch.cpp
    busy_wait.cpp
    sleep_vs_spin.cpp

numa/
    local_vs_remote_memory.cpp
    numa_binding.cpp
    numa_first_touch.cpp

networking/
    tcp_latency.cpp
    tcp_nodelay.cpp
    udp_latency.cpp
    epoll_lt.cpp
    epoll_et.cpp
    busy_poll.cpp
    send_batching.cpp
    recv_batching.cpp

data_structure/
    vector_vs_list.cpp
    map_vs_unordered_map.cpp
    flat_map.cpp
    array_of_struct_vs_struct_of_array.cpp
    intrusive_container.cpp

orderbook/
    naive_orderbook.cpp
    optimized_orderbook.cpp
```

tools/
run_benchmark.py
run_perf.py
cpu_info.sh
environment_report.sh

results/
raw/
processed/

scripts/
build_release.sh
run_all.sh

---

## 三、Benchmark Infrastructure

先实现统一 benchmark framework。

要求每个 benchmark 都支持：

--iterations
--warmup
--threads
--cpu
--duration

至少输出：

mean
median
min
max
p50
p90
p95
p99
p99.9
standard deviation

对于 throughput benchmark 输出：

ops/sec

对于 latency benchmark 输出：

ns/op

如果使用 CPU cycles：

cycles/op

需要区分：

Latency benchmark

与

Throughput benchmark

不要把二者混在一起。

对于极短操作，尽量使用 batch measurement：

例如连续执行 N 次操作，再除以 N，

避免 timer overhead 比测试对象本身还大。

同时测量 timer overhead。

---

## 四、CPU Timing

实现并比较：

std::chrono::steady_clock

clock_gettime

RDTSC

RDTSCP

解释：

RDTSC

RDTSCP

LFENCE

CPUID

之间的关系。

讨论：

instruction reordering

CPU frequency

TSC invariant

core migration

对 benchmark 的影响。

如果直接使用 RDTSC：

必须说明 serialization 问题。

建立：

timer overhead benchmark。

---

## 五、Cache Experiments

这是整个项目最重要的一组实验之一。

至少包含：

1. Sequential Access vs Random Access

创建不同大小数组：

4 KB
32 KB
256 KB
1 MB
4 MB
16 MB
64 MB
256 MB

观察随着 working set 超过：

L1
L2
L3

性能变化。

2. Cache Line

验证：

64-byte cache line

相关行为。

不要简单假定所有机器都是 64 字节，
运行时读取或检测，并记录环境。

3. False Sharing

建立：

struct Counter {
atomic<uint64_t> value;
};

多个线程分别更新不同 Counter。

然后比较：

普通排列

vs

alignas(64)

或：

std::hardware_destructive_interference_size

观察：

throughput
cache miss
cache coherency

尝试使用：

perf stat

分析：

cache-references
cache-misses
cycles
instructions

如果可用，观察 LLC / HITM 等事件。

4. Cache Stride

stride：

1
2
4
8
16
32
64
128

分析 cache line utilization。

5. Prefetch

比较：

normal access

vs

__builtin_prefetch

但必须强调：

prefetch 不一定更快。

研究：

prefetch distance。

---

## 六、Branch Prediction

至少做：

predictable branch

vs

random branch

例如：

if (x > threshold)

其中：

sorted data

vs

random data

比较：

branch implementation

vs

branchless implementation。

使用：

perf stat

观察：

branches
branch-misses

分析：

什么时候 branchless 反而更慢。

---

## 七、Memory Allocation

比较：

malloc / free

new / delete

固定大小 memory pool

arena allocator

std::pmr

thread local allocator

实验场景：

大量短生命周期小对象。

指标：

mean latency
p99
p99.9

特别关注：

tail latency。

解释：

为什么 HFT 系统通常避免 hot path allocation。

增加一个实验：

preallocate

vs

allocate during hot path。

---

## 八、Synchronization

至少实现：

std::mutex

spinlock

ticket lock

std::shared_mutex

atomic

比较场景：

low contention

medium contention

high contention

不同 critical section 长度。

必须避免做出：

“spinlock 比 mutex 快”

这样的简单结论。

分析：

线程数量
critical section duration
oversubscription
scheduler

对结果的影响。

---

## 九、Atomic Memory Ordering

构造实验理解：

memory_order_relaxed

memory_order_acquire

memory_order_release

memory_order_acq_rel

memory_order_seq_cst

不仅 benchmark。

还需要通过代码展示：

为什么某些 relaxed 写法是 incorrect 的。

建立一个简单 message passing：

Producer:

data = ...
flag.store(true, release)

Consumer:

flag.load(acquire)
read data

解释 happens-before。

另外 benchmark：

relaxed

vs

seq_cst

但必须谨慎解释结果。

---

## 十、Lock-Free Queue

实现：

SPSC Ring Buffer。

要求：

固定容量。

避免：

malloc

mutex

dynamic allocation。

讨论：

head
tail
memory ordering
cache line padding

避免 producer / consumer false sharing。

实现：

baseline naive SPSC

然后：

optimized SPSC

benchmark before / after。

指标：

messages/sec

ns/message

p99 latency。

之后可以再尝试：

MPSC。

但不要为了复杂而写明显不正确的 lock-free algorithm。

优先 correctness。

---

## 十一、Thread Scheduling

做实验：

unpinned thread

vs

CPU affinity。

使用：

pthread_setaffinity_np

或者：

taskset。

观察：

CPU migration。

增加：

sched_getcpu()

记录线程运行在哪个 CPU。

比较：

pinned

vs

unpinned

latency jitter。

实验：

sleep

yield

busy spin

hybrid spin + sleep。

分析：

latency

vs

CPU utilization。

---

## 十二、NUMA

如果系统是 NUMA：

使用：

numactl

libnuma

first touch。

设计：

CPU node 0 + memory node 0

vs

CPU node 0 + memory node 1

测试：

sequential memory bandwidth
random memory access
pointer chasing

输出：

local NUMA latency

vs

remote NUMA latency。

同时解释：

为什么 Quant / Low-Latency 系统需要考虑：

NIC
CPU
memory

NUMA locality。

如果运行环境不是 NUMA：

保留实验代码，
但清楚说明无法验证。

---

## 十三、Networking

建立 localhost benchmark。

至少：

TCP ping-pong

UDP ping-pong。

统计：

RTT

p50
p99
p99.9

TCP 实验：

TCP_NODELAY on/off

small message

batch message

send batching。

epoll：

实现：

LT

ET。

必须正确处理：

non-blocking

EAGAIN

partial read

partial write。

另外设计：

blocking socket

vs

epoll

vs

busy polling

但不要简单得出绝对性能结论。

---

## 十四、Data Structure Benchmark

不要只比较理论复杂度。

比较：

std::vector

std::list

std::deque

std::map

std::unordered_map

sorted vector / flat map

不同数据规模：

N=16
N=64
N=256
N=1024
N=10000
N=100000

场景：

lookup
insert
iterate
erase

重点展示：

cache locality

与理论 Big-O 的差异。

特别做：

vector traversal

vs

list traversal。

---

## 十五、AoS vs SoA

设计：

Array of Structures

vs

Structure of Arrays。

例如：

struct Order {
double price;
int quantity;
int id;
char side;
};

只遍历：

price

quantity。

比较：

AoS

vs

SoA。

观察：

cache bandwidth

vectorization。

---

## 十六、Order Book Mini Benchmark

实现一个简单 limit order book。

第一版：

naive implementation。

例如：

map<price, queue<Order>>

第二版：

针对已知 price range 做优化。

可以尝试：

flat structure
array
price-index mapping
preallocated order pool

支持：

Add
Cancel
Modify
Match

benchmark：

market data style update workload。

输入模拟：

add 60%
cancel 25%
modify 10%
trade 5%

不要把这个比例声称为真实交易所比例，
只作为 synthetic workload。

输出：

updates/sec

mean latency

p99

p99.9。

然后进行至少三轮性能优化。

每一轮必须输出：

before

after

change

why

perf evidence。

---

## 十七、Compiler Optimization

比较：

-O0
-O1
-O2
-O3
-march=native

必要时：

LTO。

分析：

instructions
cycles
IPC

但不要仅根据 runtime。

查看：

compiler generated assembly。

可使用：

objdump

或者：

Compiler Explorer 风格输出。

选择几个实验分析：

function inline

branchless

vectorization

loop unrolling。

---

## 十八、Performance Counters

建立统一：

perf stat

脚本。

至少收集：

task-clock
context-switches
cpu-migrations
page-faults
cycles
instructions
branches
branch-misses
cache-references
cache-misses

计算：

IPC = instructions / cycles

分析不同实验。

如果某些 event 在系统不可用：

不要失败。

检测并降级。

---

## 十九、Tail Latency

整个项目必须强调：

平均延迟不能代表 Low-Latency 系统。

所有 latency benchmark 尽量提供：

p50
p90
p99
p99.9
max

解释：

tail latency。

增加一个实验：

background load

例如：

CPU stress
memory stress

观察：

p99 / p99.9

变化。

---

## 二十、Benchmark Correctness

建立独立文档：

docs/measurement_pitfalls.md

必须讨论：

CPU frequency scaling
Turbo Boost
HyperThreading / SMT
CPU migration
scheduler noise
NUMA
page faults
cold cache
warm cache
compiler optimization
timer overhead
background process
thermal throttling
virtualization
container
ASLR

不要强制要求关闭所有功能。

但必须记录环境。

---

## 二十一、Environment Report

创建：

environment_report.sh

自动记录：

uname -a

lscpu

CPU model

CPU frequency

cache hierarchy

NUMA topology

compiler version

kernel version

glibc version

CMake version

perf version

当前 CPU governor

SMT 状态。

每次 benchmark 保存：

environment.json

或：

environment.txt。

---

## 二十二、Result Format

所有 benchmark 支持：

JSON

CSV

human readable table。

例如：

{
"benchmark": "false_sharing",
"threads": 4,
"iterations": 10000000,
"mean_ns": ...,
"p50_ns": ...,
"p99_ns": ...,
"p999_ns": ...,
"throughput": ...
}

Python 脚本自动生成：

Markdown summary。

暂时不需要复杂 Web Dashboard。

---

## 二十三、每个实验的 README

每个 benchmark 都必须有对应说明。

统一格式：

# Experiment

## Question

这个实验回答什么问题？

## Hypothesis

测试前预期是什么？

注意：

Hypothesis 不能被写成最终结论。

## Setup

hardware
compiler
flags
threads
workload

## Baseline

baseline implementation。

## Variant

优化实现。

## Expected hardware behavior

cache
branch
memory
scheduler

## Run

具体命令。

## Result

benchmark 数据。

## perf analysis

perf 数据。

## Explanation

为什么出现这个结果？

## When does this optimization help?

## When might it hurt?

## Interview Questions

5–10 个相关面试问题。

---

## 二十四、Interview Mode

README 中增加：

Interview Notes。

对于每个主题给出：

30 秒回答

2 分钟回答

深入追问。

例如 false sharing：

30 秒回答：

什么是 false sharing？

2 分钟：

为什么不同变量仍会竞争 cache line？

Deep Dive：

MESI / coherency / HITM。

实验：

对应 benchmark。

形成：

理论
↓
代码
↓
benchmark
↓
perf
↓
面试答案

完整闭环。

---

## 二十五、第一阶段任务

不要一次性生成全部实验。

Phase 1 先完成基础设施和以下 8 个实验：

1. timer overhead
2. sequential vs random memory access
3. false sharing
4. predictable vs unpredictable branch
5. malloc vs memory pool
6. mutex vs spinlock
7. SPSC ring buffer
8. thread affinity

要求：

每一个都真正：

compile

run

test。

如果当前环境允许：

运行 benchmark。

如果当前环境允许 perf：

运行 perf。

不要伪造 benchmark 结果。

如果当前环境无法执行某些工具：

明确记录：

NOT MEASURED

而不是生成模拟数据。

---

## 二十六、Review

完成 Phase 1 后进行一次严格 review。

Review 分成：

A. C++ correctness

检查：

UB
data race
memory ordering
lifetime
alignment

B. Benchmark correctness

检查：

dead code elimination
measurement overhead
warmup
sample size
timer correctness

C. Linux correctness

检查：

affinity
perf
scheduler
NUMA

D. Low-Latency relevance

判断实验是否真实有助于理解低延迟系统。

E. Documentation

检查：

是否存在过度概括
是否将特定测试结果误认为普遍规律

发现问题必须修改。

不是只输出 review report。

---

## 二十七、工程要求

要求：

C++20

CMake

-Wall
-Wextra
-Wpedantic

Debug 支持：

AddressSanitizer
UndefinedBehaviorSanitizer
ThreadSanitizer

尽可能增加：

unit tests。

Release benchmark 禁止 sanitizer。

支持：

cmake --build

ctest。

确保 README 中给出完整 build/run instruction。

---

## 二十八、性能优化原则

后续优化代码必须遵循：

Measure
↓
Identify Bottleneck
↓
Form Hypothesis
↓
Change
↓
Measure Again

禁止：

premature optimization。

禁止：

仅凭代码看起来复杂就认为更快。

禁止：

优化多个因素后无法解释性能来源。

尽可能：

one change at a time。

---

## 二十九、最终目标

这个 repository 最终应该成为：

Low-Latency C++
+
Computer Architecture
+
Linux Performance
+
Concurrency
+
Networking
+
HFT Engineering

的实验型知识库。

它应该让我能够：

看到一个 Low-Latency 面试问题

↓

理解 CPU / OS 原理

↓

运行对应实验

↓

查看 perf

↓

看到真实性能数据

↓

理解适用边界

↓

形成面试回答。

不要把项目做成一堆 disconnected microbenchmark。

重点建立知识之间的连接。

现在开始执行：

第一步：

先输出 project architecture 和 Phase 1 implementation plan。

随后直接创建项目，不需要等待我确认。

完成 Phase 1 后：

build
test
run
benchmark
perf
review
fix

最终给出：

1. 当前 repository 状态
2. 已完成实验
3. benchmark 结果
4. 主要发现
5. measurement caveats
6. 后续 Phase 2 计划
