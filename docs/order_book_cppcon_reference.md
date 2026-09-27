# CppCon 2024 Order Book 参考与本实验边界

查阅日期：2026-09-27。David Gross 在 CppCon 2024 分享 *When Nanoseconds Matter: Ultrafast Trading Systems in C++*；官方讲义首页标明 Optiver。用户提供的 [Reddit 讨论](https://www.reddit.com/r/cpp/comments/1fqltp3/when_nanoseconds_matter_ultrafast_trading_systems/) 用于定位材料，技术依据采用 [CppCon 官方讲义](https://github.com/CppCon/CppCon2024/blob/main/Presentations/When_Nanoseconds_Matter.pdf) 和 [官方视频](https://www.youtube.com/watch?v=sX2nF1fW7kI)。下列页码是讲义印刷页码，并非 PDF 文件页序。

## 讲义中可直接核查的内容

- **语义**：示例处理交易所 Add / ModifyVolume / Delete 消息，维护买卖价位总量；ID 在交易日内唯一。它是行情簿示例，未给出本项目的主动撮合、改价失去 FIFO 优先级或 ID 复用规则。（[讲义 16–24](https://github.com/CppCon/CppCon2024/blob/main/Presentations/When_Nanoseconds_Matter.pdf)）
- **起点**：两侧有序 `map` 配合订单 ID 哈希索引；订单信息可保存稳定的价位 iterator，避免修改/删除时再次查找价位。（[讲义 25–29](https://github.com/CppCon/CppCon2024/blob/main/Presentations/When_Nanoseconds_Matter.pdf)）
- **候选优化**：有序 `vector` + `lower_bound`；把最佳价位放在末端以减少搬移。插入会使 iterator 失效，不能照搬 map 的索引方式。随后比较 branchless 二分与线性搜索，强调查看实际数据分布。（[讲义 33–54](https://github.com/CppCon/CppCon2024/blob/main/Presentations/When_Nanoseconds_Matter.pdf)）
- **测量**：较重的初始化不应混入热点 `perf`；先看 top-down 分类，再检查调用栈和硬件计数。（[讲义 44–51](https://github.com/CppCon/CppCon2024/blob/main/Presentations/When_Nanoseconds_Matter.pdf)）

## 本项目的采用方式

以下是实验设计选择，不是讲者的原始实现或真实交易所规范：

1. 本轮建立 `map + unordered_map + list` baseline。`list` 保存逐单 FIFO，服务于本项目的价格时间优先撮合；ID 哈希索引从 baseline 起就存在，因此不能再把“新增 ID 哈希”计作后续独立优化。
2. 用独立的扫描式 reference 逐事件比较状态码、全部成交、剩余量及 FIFO。性能 checksum 只防止结果被丢弃，不能替代语义对照。
3. 后续按实测瓶颈选择价位目录、分配器或搜索算法；每轮只改一个主要因素，保留相同容量、拒绝和输出语义。若采用有序 vector，先解决订单索引对价位移动的引用有效性。
4. 每次优化记录 before/after 的独立进程测量、相同事件流与构建信息，画单事件尾延迟和批量吞吐图。展示重复运行离散度，保留变慢结果。三次重复测量不算三轮优化。

讲义中的延迟数值不移植为本仓库结果；当前教学撮合工作负载也不能据此宣称重现 Optiver 系统性能。讲义建议中的“最佳价位在末端”是否适用于本实验，需要另测价格访问分布和价位插入/删除次数。
