#!/usr/bin/env bash
# Read-only: no governor, sysctl, isolation or affinity changes.
set -u
export LC_ALL=C
run() { printf '\n## %s\n' "$*"; "$@" 2>&1 || true; }
run date -u +%FT%TZ
run uname -a
run lscpu
run lscpu -e=CPU,NODE,SOCKET,CORE,ONLINE,MAXMHZ,MINMHZ
run numactl --hardware
run getconf GNU_LIBC_VERSION
run g++ --version
run clang++ --version
run cmake --version
root=$(cd "$(dirname "$0")/.." && pwd)
if [[ -x "$root/.tools/bin/cmake" ]]; then run "$root/.tools/bin/cmake" --version; fi
run perf --version
run taskset -pc $$
run cat /proc/sys/kernel/perf_event_paranoid
run cat /proc/sys/kernel/randomize_va_space
run cat /sys/devices/system/cpu/smt/active
run cat /sys/devices/system/cpu/cpufreq/boost
run cat /proc/loadavg
run cat /proc/meminfo
run cat /proc/sys/kernel/numa_balancing
run cat /sys/kernel/mm/transparent_hugepage/enabled
run cat /sys/kernel/mm/transparent_hugepage/defrag
run cat /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
run cat /sys/kernel/mm/hugepages/hugepages-2048kB/free_hugepages
run cat /proc/self/cgroup
run cat /proc/self/status
for path in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
  [[ -f "$path" ]] && printf '%s: %s\n' "$path" "$(cat "$path")"
done
for path in /sys/devices/system/cpu/cpu0/cache/index*/{level,type,size,coherency_line_size,shared_cpu_list}; do
  [[ -f "$path" ]] && printf '%s: %s\n' "$path" "$(cat "$path")"
done
exit 0
