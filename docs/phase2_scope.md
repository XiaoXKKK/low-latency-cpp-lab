# Phase 2 本轮范围（用户确认）

用户选择分批推进：本轮完成路线图前四组（测量深化、Cache/Memory、同步调度、NUMA）。网络、数据结构/AoS-SoA、order book 留在后续批次；MPSC 是可选后续项，本轮不引入未经证明的多生产者算法。

验收：
- 保留 Phase1 八项实验；公共结果支持显式 NOT MEASURED 与不同单位，不输出伪造的零测量。
- 测量：绝对到达时间的开放计划与背景CPU/内存负载、服务时间/响应时间分离；TSC_AUX过滤和明确tick单位；区间perf cycles/instructions/IPC；编译级别/native/LTO矩阵与assembly。
- Cache：streaming read bandwidth、stride 1..128、prefetch distance 对照。
- Memory：批量短生命周期分配、arena/pmr/thread-local、自线程/跨线程free；mmap/first touch/warm touch；THP与显式hugepage验证。
- 同步调度：ticket/shared_mutex、所有指定atomic memory order的合法操作；release/acquire发布正确性；sleep/yield/spin/hybrid，记录线程CPU时间。
- NUMA：指定 CPU 与目标memory node、strict binding及first touch；实际页位置前后查询；顺序与random/pointer chase，仅验证成功才标local/remote。
- 三轮实测、Release与sanitizer tests、严格review及修复、中文说明/面试问答/真实数据报告。
