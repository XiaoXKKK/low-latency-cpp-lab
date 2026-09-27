#!/usr/bin/env python3
"""Preserve each implementation's independent process runs for later plots."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--binary', type=Path, default=ROOT / 'build/release/lab_bench')
    p.add_argument('--variant', default='map_list')
    p.add_argument('--label', default='map + hash + list baseline')
    p.add_argument('--cpu', default=str(min(os.sched_getaffinity(0))))
    p.add_argument('--sizes', default='16,256,4096')
    p.add_argument('--events', type=int, default=20000)
    p.add_argument('--samples', type=int, default=30)
    p.add_argument('--repeats', type=int, default=3)
    p.add_argument('--seed', type=int, default=42)
    p.add_argument('--warmup-events', type=int, default=1000)
    p.add_argument('--warmup-replays', type=int, default=3)
    a = p.parse_args()
    try:
        sizes = sorted(set(int(n) for n in a.sizes.split(',')))
    except ValueError:
        p.error('sizes must be comma-separated integers')
    if not sizes or not all(1 <= n <= 10000 for n in sizes) or not 1 <= a.events <= 100000:
        p.error('initial orders 1..10000, events 1..100000 required')
    if a.samples < 1 or a.repeats < 3 or not 0 <= a.warmup_events <= 100000 or not 0 <= a.warmup_replays <= 100000:
        p.error('positive samples, at least three independent repeats, bounded nonnegative warmup required')
    if ',' in a.cpu:
        p.error('single CPU required for order book campaign')
    output = a.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    campaign = {'schema_version': 1, 'label': a.label, 'variant': a.variant, 'runs': [],
                'script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    jobs = [(n, mode) for n in sizes for mode in ['latency', 'throughput']]
    random.Random(a.seed).shuffle(jobs)
    try:
        for n, mode in jobs:
            directory = f'n{n}-{mode}'
            command = [sys.executable, str(ROOT / 'tools/run_benchmark.py'), '--binary', str(a.binary.resolve()),
                       '--benchmark', f'order_book_{mode}', '--variant', a.variant, '--cpu', a.cpu,
                       '--size', str(n), '--batch', str(a.events), '--seed', str(a.seed),
                       '--iterations', str(a.events if mode == 'latency' else a.samples),
                       '--warmup', str(a.warmup_events if mode == 'latency' else a.warmup_replays),
                       '--repeats', str(a.repeats), '--output', str(output / directory)]
            record = {'size': n, 'mode': mode, 'directory': directory, 'command': command, 'status': 'FAILED'}
            campaign['runs'].append(record)
            subprocess.run(command, check=True)
            record['status'] = 'COMPLETE'
    finally:
        (output / 'campaign.json').write_text(json.dumps(campaign, indent=2) + '\n')


if __name__ == '__main__':
    main()
