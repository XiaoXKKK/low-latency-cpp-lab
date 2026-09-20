# Networking 本轮验收审查

范围：在首个公开提交 `292f386` 之上，补齐 perf 报告与 GCC/Clang 矩阵，完成原需求第十三节 networking。以下为实现者逐项自查与自动化测试证据，不声称独立审查。

| 轴 | 核对项 | 证据 / 边界 |
|---|---|---|
| Standards | C++20、编译告警、Release 与 sanitizer 分离 | CMake 统一配置；Release、ASan+UBSan、TSan 日志 |
| Standards | socket/线程所有权与退出 | Fd RAII；两端分属不同线程；stop release/acquire；shutdown 后 join；错误在 join 后读取 |
| Standards | 一次改变一个主要变量 | NODELAY 开关、同 window 合并 send、相同 TCP 协议的等待模式对照 |
| Standards | 可重复、原始数据、单位 | 统一 runner 三轮、seed、CPU、编译信息、源码/二进制哈希；单次 RTT 与 window mean 区分 |
| Spec | TCP/UDP loopback ping-pong | network_rtt TCP/UDP RTT，逐字节校验；UDP sequence 与未回复统计 |
| Spec | 小消息 / batch / NODELAY | payload 和 window sweep，unbatched/batched 对照；不把 UDP burst 称为 sendmmsg |
| Spec | blocking / epoll LT / ET / busy | network_io 共四变体；明确 busy 是用户态重试 |
| Spec | nonblocking / EAGAIN / partial I/O | 小发送缓冲区 1 MiB 传输与 EOF、背压、超时测试；ET 连续帧与重新到达测试 |
| Spec | p50 / p99 / p99.9 / CPU | schema v2 原始样本与分位数；process CPU / wall；RTT 主实验每轮 10000 个请求 |
| Spec | 编译器与 PMU | 12 个 profile，逐个 build/test/assembly/measure；PMU pair 成组，复用时报告 scaled_ipc |

检查中已修正：

- perf 成功事件残留失败 reason；原始历史文件保持不变。
- perf 全进程 IPC 未区分复用；增加 running_ns、running_percent、multiplexed，将缩放比值与完整调度计数比值分开。
- 阻塞 I/O 若始终使用固定 socket timeout，末次系统调用会额外延长剩余 deadline；现在随剩余时间更新，保留 ms 取整和调度延迟说明。
- ET 不能把一帧收完当作 EAGAIN；后续 frame 先继续 syscall，避免等待不会再次出现的 edge。

正确性测试不以“某变体必须更快”为通过条件。网络只做 localhost、单连接、两线程，没有多连接公平性/扩展性、NIC busy poll、真实链路或开放到达网络负载结论。单次实验和全进程 perf 的计数边界不同，报告不将二者强行换算为同一精确请求成本。

数据结构 / AoS–SoA、Order Book 以及 MPSC 不属于本轮交付。
