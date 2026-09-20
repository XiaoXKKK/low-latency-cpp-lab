#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
for locks in 4 2 1; do
  for critical in 0 32 256; do
    python3 "$root/tools/run_benchmark.py" --benchmark locks --threads 4 --locks "$locks" \
      --critical "$critical" --iterations 100 --warmup 10 --repeats 3 "$@"
  done
done
