# Linux measurement and tuning

先 `lscpu -e=CPU,NODE,SOCKET,CORE`、`taskset -pc $$`、`numactl --hardware`。两个不同逻辑 CPU 不保证两个物理核；同 NUMA node 不保证共享同一 LLC。`--cpu 0,1` 在并行实验给两个 worker 各一 CPU，main 保持 allowed mask。affinity 实验 unpinned 保留继承的 mask；若外层 taskset 已只给一个 CPU，就无法形成真正对照。

`pthread_setaffinity_np` 的错误码直接检查，不能错误地只读 errno。改变 affinity 不等于隔离 CPU，IRQ 和其他任务仍然可以运行。不得把“未观察到迁移”写成“调度器没有干扰”。

perf 脚本逐事件探测，保存拒绝原因；如果没有任何可用事件，标为 NOT MEASURED。权限受内核策略和 capabilities 等因素控制，记录 perf_event_paranoid。脚本不修改 sysctl，也不使用 sudo。[Linux perf security](https://www.kernel.org/doc/html/latest/admin-guide/perf-security.html)

perf stat 统计整个子进程（含 setup/warmup/output）；它不是严格匹配 C++ 内部计时区间的计数。解释前增大 hot loop 占比、隔离一个 variant 并查看 counts 与 running percentage。Phase 2 可引入受控的 perf_event_open 区间计数。

```bash
numactl --cpunodebind=0 --membind=0 ./build/release/lab_bench --benchmark memory_access --size 67108864
numactl --cpunodebind=0 --membind=1 ./build/release/lab_bench --benchmark memory_access --size 67108864
```

以上仅为未来 NUMA 对照入口：是否允许 membind、实际页落在哪个 node 必须另行验证，Phase 1 不报告 local/remote 实测结论。NIC 的 PCIe NUMA node、IRQ CPU、packet processing CPU、内存节点共同构成后续网络实验的 locality 链。
