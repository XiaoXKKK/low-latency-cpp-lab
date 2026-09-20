# Architecture and acceptance

Phase 1 边界：八个实验，独立 correctness tests，统一输入/输出，真实多轮数据和诚实的环境限制。完整用户需求保留于 project_request.md；广泛的 networking/orderbook/NUMA 等要求是长期路线，不一次生成占位 microbenchmark。

公共框架负责 Config 验证、统计、结果序列化、计时、常驻 worker 和 affinity。实验负责工作负载、setup、正确性校验和解释 notes。Python 负责跨进程随机化、多轮、环境/源码哈希/flags 记录与 profiler。文档负责解释适用边界，原始数据不会被手工改成“理想结果”。

实施阶段：
1. 构建与测量框架；区分 latency 与 throughput。
2. 八项 baseline/variant，SPSC 两方向发布证明，pool 容量约束。
3. build/CTest、ASan+UBSan、TSan；工具不可执行与代码失败分别记录。
4. 至少三轮，容量 sweep，足量逐次分配样本；profiler 权限探测。
5. Standards/Spec 双轴审查，再按 C++/measurement/Linux/relevance/docs 五类核对并修复。

扩展接口保持简单：新函数 `Results experiment(const Config&)`，注册到 main、CMake、runner 和 CLI 测试。只有共享语义稳定后再抽象，避免将不同的样本单位塞到统一但含糊的 latency 列中。

## Phase 2 modules

结果序列化独立到src/results.cpp；Linux页映射/归属查询放到linux_memory模块，Arena与TicketLock分别独立头文件。新增11组实验继续通过现有Config/Result接口集成，schema v2可表示NOT MEASURED和附加metrics。第一批范围及后续边界见phase2_scope.md；真实能力不足由结果状态表达，代码错误仍失败。
