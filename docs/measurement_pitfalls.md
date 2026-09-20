# Measurement pitfalls

| 因素 | 可能偏差 | 本项目如何处理 / 未解决部分 |
|---|---|---|
| CPU frequency scaling / Turbo Boost | 同样指令的 ns 随频率变化 | 记录型号、频率范围、governor、boost；不自动关闭 |
| HyperThreading / SMT | sibling 竞争执行资源 | 记录 core/socket/node，示例选不同物理核；SMT 保留 |
| CPU migration | cache 冷启动、TSC/拓扑变化 | 明确 allowed mask、可绑核；affinity 采样 sched_getcpu |
| scheduler noise | 抢占放大 tail，屏障唤醒成本 | 常驻 worker，多轮结果，不删除离群值 |
| NUMA | first touch 决定初始页放置，跨节点增加成本 | 记录拓扑；Phase 1 不把内存位置宣称为已验证 |
| page faults | 首次触页远高于热访问 | memory setup 遍历完整 cycle；allocator 单对象热缓存；冷启动另立实验 |
| cold cache / warm cache | 测试工作集状态不同 | 写明预热；不声称 warmup 让 256 MiB 全驻 cache |
| compiler optimization | DCE、常量折叠、hoisting、if conversion | runtime input、barrier、noinline、branch flags、objdump |
| timer overhead | 单次几十 ns 的操作容易被 timer 主导 | timer 控制组、batch；逐次样本保留开销不硬减 |
| background processes | IRQ、其他任务造成时变干扰 | 环境 loadavg、三轮；Phase 2 扩展 background stress |
| thermal throttling | 长跑频率下降 | 记录可用频率信息，未连续读温度则温度影响 NOT MEASURED |
| virtualization / container | vCPU 停顿、虚拟 PMU、cpuset/cgroup 限制 | lscpu、cgroup、status；探测失败不冒充裸机证据 |
| ASLR | 布局改变、aliasing、随机性 | 记录 randomize_va_space，多进程复测，不自动关闭 |
| NUMA automatic balancing | 页可能迁移 | 环境不等于实际页归属，未来 numa_maps 验证 |
| perf multiplexing | events 太多导致缩放和误差 | 保存 perf CSV 与原始 event running%，IPC 谨慎解释 |
| percentile resolution | n=100 不能推断 p999 SLA | 保留 n、sample_kind；allocation 扩大到 100000 |
| coordinated omission | 闭环在停顿时停止发请求 | SPSC 为 closed-loop；不把结果视为真实交易所到达模型 |

不要强制关闭所有系统功能来得到“漂亮数字”。若要改变系统设置，先记录原值、限定实验、说明恢复方式。在共享服务器上优先选择允许 CPU、较短测量和明确的边界。
