#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
python3 "$root/tools/run_benchmark.py" --suite phase2 --repeats 3 --perf "$@"
