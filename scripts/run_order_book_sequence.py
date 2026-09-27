#!/usr/bin/env python3
"""Interleave all variants by independent process round, then export plot campaigns."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
LABELS = {
    'map_list': 'map + hash + list',
    'map_slots_ordered': 'map (ordered level slots)',
    'map_slots_random': 'map (randomized level slots)',
    'vector_front': 'vector + lower_bound (best front)',
    'vector_back': 'vector + lower_bound (best back)',
    'vector_branchless': 'vector + branchless (best back)',
    'vector_linear': 'vector + linear (best back)',
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build/release/lab_bench')
    parser.add_argument('--cpu', type=int, default=min(os.sched_getaffinity(0)))
    parser.add_argument('--events', type=int, default=20000)
    parser.add_argument('--samples', type=int, default=30)
    parser.add_argument('--repeats', type=int, default=3)
    parser.add_argument('--seed', type=int, default=42)
    args = parser.parse_args()
    if not 1 <= args.events <= 100000 or args.samples < 1 or args.repeats < 3:
        parser.error('events 1..100000, positive samples and >=3 independent repeats required')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    jobs = [(size, mode) for size in [16, 256, 4096] for mode in ['latency', 'throughput']]
    random.Random(202409).shuffle(jobs)  # Schedule seed distinct from event/layout seeds.
    sequence = {'schema_version': 1, 'schedule_seed': 202409, 'event_seed': args.seed,
                'allocation_seed': 1729, 'script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                'labels': LABELS, 'jobs': [], 'status': 'RUNNING'}
    campaigns = {v: {'schema_version': 1, 'variant': v, 'label': label, 'runs': []} for v, label in LABELS.items()}
    try:
        for size, mode in jobs:
            directory = f'n{size}-{mode}'
            group = output / 'interleaved' / directory
            # No --variant: run_benchmark randomizes the seven variants within
            # EACH repeat and launches a separate process for every measurement.
            command = [sys.executable, str(ROOT / 'tools/run_benchmark.py'), '--binary', str(args.binary.resolve()),
                       '--benchmark', f'order_book_{mode}', '--cpu', str(args.cpu), '--size', str(size),
                       '--batch', str(args.events), '--iterations', str(args.events if mode == 'latency' else args.samples),
                       '--warmup', '1000' if mode == 'latency' else '3', '--repeats', str(args.repeats),
                       '--seed', str(args.seed), '--output', str(group)]
            record = {'size': size, 'mode': mode, 'command': command, 'status': 'FAILED'}
            sequence['jobs'].append(record)
            subprocess.run(command, check=True)
            manifest = json.loads((group / 'manifest.json').read_text())
            for variant, campaign in campaigns.items():
                records = [r for r in manifest['runs'] if r['variant'] == variant]
                if len(records) != args.repeats or any(r['status'] != 'MEASURED' for r in records):
                    raise RuntimeError(f'incomplete measurements for {variant}')
                target = output / variant / directory
                target.mkdir(parents=True)
                selected = dict(manifest, runs=records)
                selected['interleaved_source'] = str(group)
                (target / 'manifest.json').write_text(json.dumps(selected, indent=2) + '\n')
                for name in ['environment.txt', 'CMakeCache.txt', 'compile_commands.json'] + [r['file'] for r in records]:
                    shutil.copy2(group / name, target / name)
                campaign['runs'].append({'size': size, 'mode': mode, 'directory': directory, 'status': 'COMPLETE'})
            record['status'] = 'COMPLETE'
        sequence['status'] = 'COMPLETE'
    finally:
        (output / 'sequence.json').write_text(json.dumps(sequence, indent=2) + '\n')
        for variant, campaign in campaigns.items():
            folder = output / variant
            folder.mkdir(exist_ok=True)
            (folder / 'campaign.json').write_text(json.dumps(campaign, indent=2) + '\n')


if __name__ == '__main__':
    main()
