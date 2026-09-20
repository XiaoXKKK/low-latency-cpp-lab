#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
for size in 4096 32768 262144 1048576 4194304 16777216 67108864 268435456; do
  python3 "$root/tools/run_benchmark.py" --benchmark memory_access --size "$size" \
    --repeats 3 --iterations "${ITERATIONS:-3}" --warmup 1 "$@"
done
