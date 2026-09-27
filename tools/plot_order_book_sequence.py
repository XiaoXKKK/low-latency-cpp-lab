#!/usr/bin/env python3
"""Render the measured CppCon sequence; never substitute lecture numbers."""
import argparse
import json
from pathlib import Path
from statistics import median
import subprocess
import sys

from plot_order_book import load_campaign

PAIRS = [
    ('00-layout', 'map_slots_ordered', 'map_slots_random'),
    ('01-vector', 'map_list', 'vector_front'),
    ('02-reverse', 'vector_front', 'vector_back'),
    ('03-branchless', 'vector_back', 'vector_branchless'),
    ('04-linear', 'vector_branchless', 'vector_linear'),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--campaign', type=Path, required=True, help='run_order_book_sequence output root')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    sequence = json.loads((args.campaign / 'sequence.json').read_text())
    if sequence['status'] != 'COMPLETE':
        parser.error('incomplete sequence')
    args.output.mkdir(parents=True, exist_ok=False)
    results = {}
    for variant in sequence['labels']:
        _, groups = load_campaign(args.campaign / variant)
        rows = {}
        for (size, mode), entries in groups.items():
            selected = {}
            for key in (['mean', 'p50', 'p99', 'p999'] if mode == 'latency' else ['ops_per_sec']):
                values = [e['row'][key] for e in entries]
                selected[key] = {'median': median(values), 'min': min(values), 'max': max(values), 'rounds': values}
            if mode == 'throughput':
                for key in ['cycles_per_event', 'instructions_per_event', 'branches_per_event', 'branch_misses_per_event', 'cache_misses_per_event', 'pmu_running_ratio']:
                    values = [e['row']['metrics'].get(key) for e in entries]
                    selected[key] = None if None in values else {'median': median(values), 'min': min(values), 'max': max(values), 'rounds': values}
            rows.setdefault(size, {})[mode] = selected
        results[variant] = rows
    lines = ['# Measured per-step comparisons', '', 'Median of three independent-process statistics; ratios are descriptive, not confidence intervals.', '',
             '| Step | Initial orders | Mean ns before → after | p99 ns before → after | M events/s before → after | Throughput change |',
             '|---|---:|---:|---:|---:|---:|']
    for stage, before, after in PAIRS:
        subprocess.run([sys.executable, str(Path(__file__).with_name('plot_order_book.py')),
                        '--campaign', str(args.campaign / before), '--campaign', str(args.campaign / after),
                        '--output', str(args.output / stage)], check=True)
        for size in sorted(results[before]):
            a, b = results[before][size], results[after][size]
            rate_a, rate_b = [x['throughput']['ops_per_sec']['median'] for x in (a, b)]
            cells = [stage, str(size)]
            for key in ['mean', 'p99']:
                cells.append(f"{a['latency'][key]['median']:.2f} → {b['latency'][key]['median']:.2f}")
            cells += [f'{rate_a/1e6:.3f} → {rate_b/1e6:.3f}', f'{100*(rate_b/rate_a-1):+.2f}%']
            lines.append('| ' + ' | '.join(cells) + ' |')
    (args.output / 'comparisons.json').write_text(json.dumps(results, indent=2) + '\n')
    (args.output / 'comparison-summary.md').write_text('\n'.join(lines) + '\n')


if __name__ == '__main__':
    main()
