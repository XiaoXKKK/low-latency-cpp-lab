#!/usr/bin/env python3
"""Sequential seeded size sweep; build/test first, then measure without builds."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build/release/lab_bench')
    parser.add_argument('--cpu', default=str(min(os.sched_getaffinity(0))))
    parser.add_argument('--iterations', type=int, default=100)
    parser.add_argument('--warmup', type=int, default=10)
    parser.add_argument('--batch', type=int, default=64)
    parser.add_argument('--perf', action='store_true')
    args = parser.parse_args()
    if args.iterations < 1 or args.warmup < 0 or not 1 <= args.batch <= 4096:
        parser.error('positive iterations, nonnegative warmup and batch 1..4096 required')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    records = []
    try:
        for size in [16, 64, 256, 1024, 10000, 100000]:
            command = [sys.executable, str(ROOT / 'tools/run_benchmark.py'),
                       '--binary', str(args.binary.resolve()), '--suite', 'data_structures',
                       '--cpu', args.cpu, '--size', str(size), '--batch', str(args.batch),
                       '--iterations', str(args.iterations), '--warmup', str(args.warmup),
                       '--repeats', '3', '--output', str(output / f'containers-n{size}')]
            # Whole-process PMU includes construction/reset/verification. Retain
            # that boundary explicitly; it is not a counter per timed operation.
            if args.perf and size == 1024:
                command.append('--perf')
            record = {'size': size, 'command': command, 'status': 'FAILED'}
            records.append(record)
            subprocess.run(command, check=True)
            record['status'] = 'COMPLETE'
    finally:
        (output / 'campaign.json').write_text(json.dumps(records, indent=2) + '\n')


if __name__ == '__main__':
    main()
