# GCC / Clang 工具链补测

2026-09-20，在 Clang 与 CAP_PERFMON 已启用的环境下重新测量。原 Phase 2 首批报告和安装验证原始输出保持不变。

## 验收与边界

GCC/Clang 各 O0、O1、O2、O3、O3-native、O3-lto，共 **12/12 配置完成**；每配置构建并通过 **7/7 CTest**，保存 objdump 汇编。三个 workload 各三轮，共 **36 个 campaign、108/108 MEASURED 独立进程**；所有 campaign 的源码哈希映射一致。构建、测试和测量逐组串行执行。

同一台 AMD EPYC 7C13，固定 CPU 0。GCC 与 Clang 均使用当前系统 libstdc++，此矩阵比较编译器与配置，未比较 libc++。编译器版本、flags、二进制哈希和逐轮命令见[矩阵 manifest](../results/raw/phase2-toolchain-matrix/compiler-matrix.json)及其下各 workload manifest。O3-native 结果只适用于该 ISA/机器。

## 三轮中位数

下表每列取独立轮次对应统计值的中位数；batch mean 不是单操作尾延迟。streaming 为 1 MiB 工作集的 uint64_t 累加，branch 为 65536 个随机值，递推为每样本 65536 个源代码步骤；每轮 100 样本、10 warmup。

| Compiler | Profile | Streaming ns/element | Random branch ns/value | Chain ns/source-step | Chain cycles/source-step | Chain instructions/source-step |
|---|---|---:|---:|---:|---:|---:|
| g++ | O0 | 7.855 | 4.902 | 1.307 | 4.019 | 11.010 |
| g++ | O1 | 0.369 | 3.795 | 1.301 | 4.007 | 5.004 |
| g++ | O2 | 0.373 | 3.714 | 1.305 | 4.006 | 5.004 |
| g++ | O3 | 0.333 | 3.952 | 1.308 | 4.020 | 5.004 |
| g++ | O3-native | 0.176 | 4.331 | 1.307 | 4.020 | 5.004 |
| g++ | O3-lto | 0.181 | 3.728 | 1.309 | 4.007 | 5.004 |
| clang++ | O0 | 8.209 | 5.629 | 3.311 | 10.175 | 13.008 |
| clang++ | O1 | 0.365 | 3.704 | 1.305 | 4.006 | 4.004 |
| clang++ | O2 | 0.152 | 3.588 | 0.228 | 0.505 | 0.504 |
| clang++ | O3 | 0.168 | 3.584 | 0.163 | 0.505 | 0.504 |
| clang++ | O3-native | 0.151 | 3.575 | 0.184 | 0.505 | 0.504 |
| clang++ | O3-lto | 0.162 | 3.711 | 0.164 | 0.505 | 0.504 |

## PMU 解释

全进程 perf 共 24 份，状态分布：{'MEASURED': 24}。cycles/instructions 作为一个 group 调度；其余硬件事件仍可能复用。每事件保存 running_ns、running_percent、multiplexed，100% 按 perf 的显示精度理解。发生复用或缺失覆盖信息时 ipc=null，只提供 scaled_ipc。全进程范围包括启动、分配、warmup，与 C++ 区间计数边界不同。

区间 perf_interval 仅记录调用线程，排除 kernel/hypervisor 和 warmup；只有 enabled==running 时才报告 cycles/source-step。计数区间包括计时器读取和控制开销，空区间已有独立实验，不做未经验证的减法校正。

## 汇编解释

[本次汇编摘录](evidence/network/compiler-assembly.md)说明一个关键差异：Clang O2 将 8 步线性递推合并成一次等价乘加；GCC O2 保留逐步循环。因此 Clang 的低 cycles/source-step 不代表同一条整数乘法延迟更低，也不能把这个结果外推为一般 C++ 程序加速比。

streaming 的 SIMD 指令和 native 指令宽度有对应汇编证据；不同 profile 的实际运行时间也受频率和缓存状态影响，LTO/native 不保证每个 workload 都更快。100 个 batch 样本不足以建立稳定的单操作 p99.9 结论。

## 复现

```bash
lab-perf python3 tools/compiler_matrix.py --cpu 0 --output results/raw/toolchain-new
```

每轮禁止与其他构建/benchmark 并行；机器上其他用户的负载仍可能影响结果。
