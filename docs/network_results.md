# Networking 实测与验收

2026-09-20，完成 Phase 2 第五组：TCP/UDP loopback、NODELAY、batching、blocking/epoll LT/ET/用户态忙轮询。数据结构/AoS–SoA、Order Book 留到下一批。

## 验收

新增 network_rtt 六个变体、network_io 四个变体，接入统一 CLI/JSON/CSV 和 runner。Release、ASan+UBSan、TSan 均 **7/7 CTest 通过**；TSan 仅测试进程使用 setarch -R，正式 Release 保留 ASLR。[测试日志](evidence/network/release-tests.log)、[ASan](evidence/network/asan-tests.log)、[TSan](evidence/network/tsan-tests.log)、[自查记录](network_review.md)。完整 GCC/Clang 12 配置补测另见[报告](toolchain_results.md)。

正式网络实验共 **13 个 campaign、105/105 MEASURED 独立进程**，每变体三轮；所有 campaign 的二进制 SHA256 与源码哈希映射一致。[campaign 命令](../results/raw/network-complete/campaign.json)、[baseline manifest](../results/raw/network-complete/baseline/manifest.json)、[环境](../results/raw/network-complete/baseline/environment.txt)。

AMD EPYC 7C13，CPU 0/1 是同 socket、同 NUMA node 的不同物理核；同核对照两线程均绑 CPU 0。IPv4 127.0.0.1 临时端口，无外部网络流量。没有修改 governor/SMT/全局 socket 参数；同机其他用户负载仍可能影响数据。构建与本轮测量不重叠。

## 读表规则

各数值为三轮对应统计量的中位数，原始样本不跨轮拼接。RTT 的 p99 是成功请求 RTT；batch 的 p99 是 window mean，不能视为单消息 p99。CPU cores = 测量循环的进程 CPU / wall，包含两个线程和样本间校验，不是单线程百分比。完整指标、p99.9、原始样本保存在 JSON；10000 次请求中 p99.9 只有约 10 个尾部样本，解释需保守。

## 64 字节基线

每轮 10000 样本、200 warmup，batch 变体 window=16；超时 200 ms。

| Benchmark / Variant | Sample unit | Mean | p50 | p99 | p99.9 | CPU cores | Completed msg/s |
|---|---|---:|---:|---:|---:|---:|---:|
| network_io/blocking | us/RTT | 22.805 | 21.270 | 33.134 | 97.983 | 1.122 | — |
| network_io/busy_poll | us/RTT | 13.768 | 13.246 | 18.976 | 33.865 | 1.999 | — |
| network_io/epoll_et | us/RTT | 23.413 | 22.763 | 31.069 | 64.976 | 1.171 | — |
| network_io/epoll_lt | us/RTT | 23.548 | 23.034 | 32.002 | 50.139 | 1.184 | — |
| network_rtt/tcp_batched | us/sent_message (window mean) | 1.376 | 1.337 | 1.807 | 3.907 | 1.121 | 726496 |
| network_rtt/tcp_default | us/RTT | 22.154 | 21.211 | 30.127 | 56.908 | 1.119 | — |
| network_rtt/tcp_nodelay | us/RTT | 21.655 | 21.111 | 29.496 | 59.323 | 1.125 | — |
| network_rtt/tcp_unbatched | us/sent_message (window mean) | 8.390 | 8.201 | 12.348 | 21.540 | 1.646 | 119186 |
| network_rtt/udp_batch | us/sent_message (window mean) | 6.619 | 6.485 | 8.901 | 18.528 | 1.836 | 151080 |
| network_rtt/udp_rtt | us/RTT | 17.695 | 17.163 | 23.615 | 66.266 | 1.087 | — |

## 同核等待策略

每轮 2000 样本、100 warmup，消息 64 字节，两线程共享 CPU 0；协议和消息大小保持相同。采样长度与双核基线不同，尾部分位数的统计精度也不同。

| Variant | Mean us/RTT | p99 us/RTT | CPU cores |
|---|---:|---:|---:|
| blocking | 24.080 | 33.826 | 0.999 |
| busy_poll | 6012.690 | 7000.684 | 1.000 |
| epoll_et | 26.299 | 37.571 | 0.999 |
| epoll_lt | 27.336 | 67.768 | 1.000 |

## 消息大小

512/4096 字节各每轮 1000 样本、100 warmup，window=16；其他设置与基线相同。

| Payload bytes | Variant | Sample unit | Mean | p99 | CPU cores |
|---:|---|---|---:|---:|---:|
| 512 | tcp_batched | us/sent_message (window mean) | 1.546 | 2.082 | 1.106 |
| 512 | tcp_default | us/RTT | 21.788 | 29.659 | 1.120 |
| 512 | tcp_nodelay | us/RTT | 22.390 | 29.852 | 1.117 |
| 512 | tcp_unbatched | us/sent_message (window mean) | 10.017 | 13.035 | 1.454 |
| 512 | udp_batch | us/sent_message (window mean) | 6.751 | 7.634 | 1.825 |
| 512 | udp_rtt | us/RTT | 18.110 | 24.188 | 1.079 |
| 4096 | tcp_batched | us/sent_message (window mean) | 3.157 | 3.789 | 1.180 |
| 4096 | tcp_default | us/RTT | 23.185 | 30.599 | 1.117 |
| 4096 | tcp_nodelay | us/RTT | 23.817 | 30.821 | 1.120 |
| 4096 | tcp_unbatched | us/sent_message (window mean) | 11.449 | 15.955 | 1.398 |
| 4096 | udp_batch | us/sent_message (window mean) | 7.314 | 9.520 | 1.714 |
| 4096 | udp_rtt | us/RTT | 20.147 | 27.151 | 1.061 |

## 发送窗口

每轮 1000 样本、100 warmup，64 字节消息。TCP 两个变体均开启 NODELAY，服务端读取整窗后回显；合并 send 只改变客户端发送调用粒度。UDP burst 每个 datagram 仍单独 send。

| Window | Variant | Mean us/sent_message | Completed msg/s | Client send calls/window | CPU cores |
|---:|---|---:|---:|---:|---:|
| 1 | tcp_unbatched | 20.999 | 47621 | 1.000 | 1.122 |
| 1 | tcp_batched | 22.095 | 45259 | 1.000 | 1.118 |
| 1 | udp_batch | 17.199 | 58144 | 1.000 | 1.101 |
| 8 | tcp_unbatched | 9.689 | 103206 | 8.000 | 1.541 |
| 8 | tcp_batched | 2.843 | 351704 | 1.000 | 1.123 |
| 8 | udp_batch | 7.268 | 137584 | 8.000 | 1.722 |
| 64 | tcp_unbatched | 7.433 | 134544 | 64.000 | 1.762 |
| 64 | tcp_batched | 0.368 | 2716686 | 1.000 | 1.117 |
| 64 | udp_batch | 5.972 | 167446 | 64.000 | 1.934 |

## UDP、perf 与计时边界

本轮共发出 2637000 条测量阶段消息，未回复 0 条（warmup 不计）；单独的 duplicate/stale/out-of-order 计数也在原始 JSON。未回复表示截止本次超时窗口未收到有效 echo，不足以定位请求或回复方向的丢包，更不代表 NIC 丢包。本机测得零未回复也不能保证 UDP 可靠。

基线额外执行 10 次全进程 perf，状态：{'MEASURED': 10}。计数包含启动、预热和两线程；cycles/instructions 成组，运行比例与 scaled_ipc 明确报告，不用全进程值直接替代每请求 PMU 成本。

sample RTT 包含发送、接收、echo 服务、系统调用和调度；payload 校验在 RTT bracket 外，但在 CPU 测量循环内。UDP 序号解析、去重和复制在 bracket 内，因此 TCP/UDP 比较也包含这些协议管理成本。阻塞 syscall 超时随剩余 deadline 更新、向上取整到 ms，内核唤醒和调度可能延后。


## 全进程 perf 对照（额外运行）

同为 64 字节、10000 次 RTT 的独立 perf 运行，包括启动和 warmup；以下为全进程原始总量，不是每请求区间 PMU。

| Variant | Context switches | Cycles (million, scaled) | Instructions (million, scaled) | Cycles running % | Scaled IPC |
|---|---:|---:|---:|---:|---:|
| blocking | 20405 | 914.767 | 674.495 | 80.00 | 0.737 |
| epoll_lt | 20402 | 1107.668 | 772.147 | 78.00 | 0.697 |
| epoll_et | 20403 | 1053.916 | 754.628 | 80.00 | 0.716 |
| busy_poll | 4 | 874.066 | 734.052 | 80.00 | 0.840 |

## 本机结果解释

64 字节、window=16 时，TCP 合并发送的批次均值为 1.376 us/message，逐条发送为 8.390 us/message；window=64 时客户端 send 调用分别为 1 次与 64 次。这支持“已有窗口合并发送可降低系统调用开销”的解释，未包含凑批等待。

双核 busy polling 平均 RTT 13.768 us、占用 1.999 个逻辑核；blocking 为 22.805 us、1.122 核。同核 busy polling 平均 RTT 6012.690 us，而 blocking / epoll 约 24–27 us。这个结果与忙重试竞争对端 CPU 时间的机制一致，不能推广为所有拓扑上的排序。

另外执行了 3 个大 UDP window 的拥塞诊断（4096 字节 × 256，20 ms 超时），768 条请求中 78 条完成、690 条未回复，验证超时路径仍保存未回复数和已完成吞吐。[诊断原始输出](evidence/network/udp-loss-validation.json)独立于上述 105 次正式测量，不与成功 RTT 混合。

## 结论边界

- NODELAY 开关只代表这里的单次完整消息写入、闭环 echo；不能推广到碎片小 write 或所有 TCP 协议。
- window batching 的吞吐提升不包含等待凑批的排队成本，不能据此断言真实单消息延迟降低。
- epoll 测的是一条连接的等待方式，不证明高连接数的扩展性。ET 与 LT 共用正确的短 I/O 路径，是否更快取决于具体负载和调度。
- 用户态 busy retry 与 SO_BUSY_POLL 不同；同核时主动重试会竞争对端所需的执行时间。
- loopback 排除了网卡、交换机与真实链路拥塞；这些结果不代表线上端到端网络延迟。

## 复现

```bash
./scripts/build_release.sh
lab-perf python3 scripts/run_network_campaign.py --cpu 0,1 --output results/raw/network-new
```

参数、协议、正确性与官方接口参考见[实验 README](../benchmarks/network/README.md)。
