# CPU architecture：从计时到队列

CPU 指令可以乱序执行；编译器也能独立重排。GNU asm compiler barrier 约束编译器，不能代替硬件 fence。`RDTSC` 本身不序列化；Intel 对有序读取给出 LFENCE 等约束。`RDTSCP` 对之前的指令/loads 有次序要求，但不是完整的 store serialization，也不阻止后续指令提前执行；配合 LFENCE 仍须按目标 CPU 文档核实。`CPUID` 是较重的序列化指令，可作更保守测量方案但成本本身会扰动结果。[Intel SDM](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)

本机为 AMD，不能从 Intel 的 LFENCE 文档推断所有 AMD 代际及 MSR 配置一致。本实验只比较读取序列本身的 ns 成本，不用未经核实的 fence 序列给任意工作负载计 cycles。TSC invariant 表示其速率不随通常 P/C/T 状态变化；不等于当前核心时钟频率，也不能独自证明跨 socket 完全同步。迁核、虚拟化的偏移/缩放都需额外验证。

memory_access 用“下一地址依赖上一 load”的闭合链。顺序链能利用空间局部性/预取，随机链暴露依赖访存等待；这与独立多路 load 吞吐不同。working set 跨过 cache/TLB 覆盖范围时，延迟可能改变；共享 cache、硬件 prefetch、页大小使边界不一定像阶梯。runtime cache line 检测来自 Linux sysconf/sysfs。

false sharing 让每个线程写不同的 atomic，但 packed 元素可能位于同一个一致性单元。padding 只改变间距；RMW 类型和内存序保持一致。HITM/所有权迁移的具体事件依 CPU 而定；通用 cache-misses 不能直接证明 false sharing。padding 同时增加 footprint，这是不可忽略的代价。

SPSC 中 producer 写槽位，然后 release 发布 head；consumer acquire 观察发布再读槽位。consumer 读完后 release 发布 tail，producer acquire 观察空位才可覆盖。acquire 需要读取对应 release（或适当 release sequence）才建立 synchronizes-with。relaxed 只保证该 atomic 访问的原子性，不自动发布旁边的普通内存。[C++ 工作草案 atomics.order](https://eel.is/c++draft/atomics.order)

```cpp
// 正确的单次发布；测试中执行这一模式
int data = 0;
std::atomic<bool> flag{false};
// Producer: data = 42; flag.store(true, std::memory_order_release);
// Consumer: while (!flag.load(std::memory_order_acquire)) {} use(data);
// 错误示意（不要运行）：把两端都改为 relaxed，普通 data 的读写缺乏 HB，产生 data race/UB。
```

“x86 看起来能运行”不构成 C++ 内存模型的证明。padding 也不修复 memory ordering 错误。
