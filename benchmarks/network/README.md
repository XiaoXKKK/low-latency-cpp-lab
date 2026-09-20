# Loopback networking：TCP/UDP、batching 与等待方式

## Question / Hypothesis

小消息回显的 RTT 有多少来自系统调用、唤醒与调度？TCP_NODELAY、合并 write、epoll LT/ET、用户态忙重试分别改变什么？假设合并 write 可降低每消息系统调用数；忙重试可能降低跨核唤醒成本，但增加 CPU 占用，同核时还可能拖慢对端。

## Setup / Baseline / Variant

两个常驻线程，一条 IPv4 loopback 连接，绑定 `127.0.0.1:0` 获取临时端口；不监听外部地址。连接、分配、绑核在采样前完成。客户端使用 CPU 列表第一个 CPU，服务端使用第二个；只提供一个 CPU 时故意同核运行。

| Benchmark | Baseline | Variants | 控制条件 |
|---|---|---|---|
| network_rtt | tcp_default | tcp_nodelay | 1 个 outstanding 请求；仅改变两端 NODELAY |
| network_rtt | tcp_unbatched | tcp_batched | 相同 window、NODELAY 开启；按消息 send vs 连续窗口 send |
| network_rtt | udp_rtt | udp_batch | 一次一个 vs burst window；UDP burst 仍逐包 send，不是 sendmmsg |
| network_io | blocking | epoll_lt / epoll_et / busy_poll | 同一 TCP_NODELAY 回显协议；仅改变两端 I/O 等待方式 |

`--size` 是消息总字节数，包含 8 字节序号，范围 8..65507；默认 64。`--batch` 仅对 window 变体生效，默认 16，最大 256，窗口最大 4 MiB。RTT 变体不使用 batch。`--timeout-ms` 默认 1000；一次操作共享绝对 deadline，阻塞 socket 超时按剩余时间向上取整到 ms，内核定时器与调度还可能延迟唤醒。

TCP 服务端读完一个 window 再回显；客户端不需要边写边读来避免互相等待。此处 window 是应用层批量回显协议，不代表通用全双工、多连接服务器。UDP 服务端逐个 datagram 回显；窗口过大可能丢包，不能假设 loopback 永不丢包。

## Ownership / Correctness

`net::Fd` 独占描述符；`net::Channel` 由单个线程使用。客户端与服务端分别拥有缓冲区与计数器，仅退出标志使用 release/acquire，共享错误在 join 后读取。异常路径 shutdown 两端后 join，正常退出不计入 RTT。

TCP 保留 short send/recv 的偏移；EINTR 重试，EAGAIN 才等待。ET 在达到一帧边界后，下次操作先继续尝试 I/O，不会误把帧完成当作 readiness 已耗尽。EPOLLOUT 只在需要写等待时订阅；EOF、HUP、ERR 通过实际 syscall 判定。发送使用 MSG_NOSIGNAL。

UDP 使用序号区分窗口，记录重复、过期和乱序回复，按 deadline 统计 unanswered。该指标说明未收到有效回显，无法定位请求还是回复丢失，也不能称为 NIC 丢包率。payload 逐字节校验失败使整次运行失败，截断报文也失败。

测试覆盖小 socket buffer 下的 1 MiB 传输、短读写、EAGAIN、背压超时、半帧 EOF、断连无 SIGPIPE、ET 连续帧与耗尽后重新到达、UDP 截断、真实 TCP/UDP CLI 与 JSON/CSV。

## Measurement / Expected hardware behavior

- 单次样本：发送前至完整 echo 收到后，单位 ns/roundtrip；闭环、仅成功 RTT，不能描述开放到达流量的排队尾部。
- window 样本：整个窗口耗时 / 发出消息数，单位 ns/sent_message；p99 是窗口均值的分位数，不能叫单条消息 p99。吞吐按完成 echo 数统计，payload bytes/sec 只算一个方向。
- sequence 填充、byte 校验在 sample 外；UDP sequence 解析、去重与复制在 sample 内。
- CPU 指标：测量循环的进程 CPU 时间 / wall time，包含两线程及校验开销；单位是占用的逻辑 CPU 数，可能超过 1。setup、warmup、teardown 排除。
- 忙轮询指用户态 EAGAIN + PAUSE 重试，不是 SO_BUSY_POLL。计数器保留 client send/recv 调用次数、short I/O、EAGAIN、epoll wait 次数。
- TCP_NODELAY 在严格 ping-pong 下未必明显获益；这里只能说明当前报文写入方式，不能推广到碎片写入和所有业务协议。

## Run

```bash
./scripts/build_release.sh
./build/release/lab_bench --benchmark network_rtt --threads 2 --cpu 0,1 --format json
./build/release/lab_bench --benchmark network_io --variant epoll_et --threads 2 --cpu 0,1 --iterations 10000
lab-perf python3 scripts/run_network_campaign.py --cpu 0,1 --output results/raw/network-new
```

先确认允许 CPU 及拓扑；复现目录须不存在。统一 runner 支持 `--suite network`；正式实验顺序执行，构建与测量不重叠。

## Result / perf analysis / Explanation

正式数据与有边界的结论见 [Networking 实测报告](../../docs/network_results.md)。三轮独立进程，环境、源码/二进制哈希、编译选项和原始样本保留。perf 全进程计数包括启动、预热及两个线程；不能直接除以样本数当作精确请求成本。复用比例与 scaled IPC 单独记录。

## When help / When hurt

合并发送可能适合已有批量消息的吞吐路径；主动等批会增加单条消息排队时间，本实验不包含该等待。忙轮询适合可留出核且强调延迟的场景；同核竞争或 CPU 预算紧张时可能有害。单连接 epoll 实验验证等待机制，不能证明大量连接下的扩展能力。loopback 只包含本机网络栈，排除了真实网卡、交换机、链路拥塞等因素。

## Interview Questions

1. ET 为什么必须在 EAGAIN 后才等待新事件？帧边界和内核缓冲区耗尽有什么区别？
2. send 成功是否代表对端已收到？为什么 TCP 必须自行定义消息边界？
3. 为什么 batch mean p99 不是请求 p99？UDP 超时样本如何影响成功 RTT 的解释？
4. 同核忙轮询为什么可能比 epoll 慢？为什么 CPU 占用需要同时报告？
5. 为什么此实验的 TCP_NODELAY 结果不能预测多次小 write 的协议行为？

## Primary references

- [Linux epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)：nonblocking、EAGAIN 与 ET readiness。
- [Linux send(2)](https://man7.org/linux/man-pages/man2/send.2.html)：short send、MSG_NOSIGNAL。
- [Linux recv(2)](https://man7.org/linux/man-pages/man2/recv.2.html)：EOF、MSG_TRUNC。
- [Linux tcp(7)](https://man7.org/linux/man-pages/man7/tcp.7.html)：TCP_NODELAY。
