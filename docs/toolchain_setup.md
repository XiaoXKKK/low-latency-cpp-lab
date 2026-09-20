# Clang 与 perf 启用记录

2026-09-19，按用户授权安装并验证。本工程路径为 `/home/ljw/low-latency-cpp-lab`。

此前 Clang 未安装；perf 程序已经存在，但 `kernel.perf_event_paranoid=4` 阻止普通用户读取计数器。因此早期报告中的 NOT MEASURED 是当时的实际状态。

## 当前配置

- 系统安装 `clang`、`llvm`、`lld`，实际版本均为 18.1.3；包版本见 [packages.txt](evidence/toolchain-enabled/packages.txt)。
- perf 沿用系统工具，版本 6.8.12。
- 全局 `perf_event_paranoid` 保持 4。仅为当前用户配置 CAP_PERFMON 启动器，运行时 UID 仍为 ljw，不以 root 执行 benchmark。
- `/home/ljw/.local/libexec/low-latency-lab/capsh` 为 root 所有、ljw 组可执行，权限 0750，文件 capability 为 `cap_perfmon=ep`。
- `/home/ljw/.local/bin/lab-perf` 为目标命令及其子进程启用 CAP_PERFMON；同目录的 `perf` 包装器通过它调用 `/usr/bin/perf`。该目录已在本机 PATH 前部。

身份、capability 与全局限制的记录见 [identity.txt](evidence/toolchain-enabled/identity.txt)、[perf-capability.txt](evidence/toolchain-enabled/perf-capability.txt)、[perf_event_paranoid.txt](evidence/toolchain-enabled/perf_event_paranoid.txt)。

## 使用

```bash
cd /home/ljw/low-latency-cpp-lab

# 普通 perf 命令直接使用，无需 sudo
perf stat -e cycles,instructions -- ./build/clang-release/lab_bench \
  --benchmark branch --variant random_branch --cpu 0 --format json

# 程序自身调用 perf_event_open 时，需要通过启动器执行
lab-perf ./build/clang-release/lab_bench --benchmark perf_interval \
  --variant dependent_chain --cpu 0 --iterations 100 --warmup 10 \
  --batch 65536 --format json

# 启动器也可用于整个 Python campaign，权限由子进程继承
lab-perf python3 tools/run_benchmark.py --binary build/clang-release/lab_bench \
  --benchmark perf_interval --cpu 0 --iterations 100 --warmup 10 \
  --batch 65536 --repeats 3 --output results/raw/my-clang-perf
```

CPU 0 在本次环境可用；其他机器应先确认允许 CPU 集合。包装器是本机配置，不随仓库或归档迁移。

## 验证结果

Clang Release 构建成功，全部 **5/5 CTest** 通过。构建配置与验证命令如下，完整日志见 [clang-build-test.log](evidence/toolchain-enabled/clang-build-test.log)。

```bash
.tools/bin/cmake -S . -B build/clang-release -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DLAB_SANITIZER=none
.tools/bin/cmake --build build/clang-release -j4
lab-perf .tools/bin/ctest --test-dir build/clang-release --output-on-failure
```

区间 PMU 实验的 empty 与 dependent_chain 各执行 3 轮，共 **6/6 MEASURED**，保存了 cycles、instructions 与 enabled/running 时间；见 [原始结果与环境](../results/raw/toolchain-enabled-clang-perf/manifest.json) 和 [汇总](../results/raw/toolchain-enabled-clang-perf/summary.md)。这是安装后的功能验证；后续完整矩阵见[工具链补测报告](toolchain_results.md)。

全进程 `perf stat` 的 10 个事件全部采集成功，包含 cycles、instructions、branch-misses、cache-misses；见 [perf.json](../results/raw/toolchain-enabled-perf-stat/perf.json) 和 [原始计数](../results/raw/toolchain-enabled-perf-stat/perf-stat.csv)。本次部分硬件事件运行比例为 81%–84%，存在 multiplex，缩放计数及派生 IPC 仅作为工具可用性验证。JSON 中 MEASURED 事件仍残留 runner 预置的 `No numeric count in actual run` reason 字段；实际状态由 status、count 与原始 CSV 共同确认。后续 runner 已修正该字段，并增加运行比例、multiplex 与 scaled IPC；本目录保留原始输出。

此前 `phase2-complete` 的结果和归档保留不变，不将新能力追溯写入历史测量。
