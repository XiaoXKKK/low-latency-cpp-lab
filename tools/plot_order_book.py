#!/usr/bin/env python3
"""Plot only measured runs; refuse different traces or incompatible settings."""
import argparse
import hashlib
import json
from pathlib import Path
from statistics import median


def load_campaign(path):
    path = Path(path).resolve()
    campaign = json.loads((path / 'campaign.json').read_text())
    if campaign.get('schema_version') != 1 or not campaign['runs']:
        raise ValueError('empty or unsupported campaign')
    groups = {}
    for job in campaign['runs']:
        if job['status'] != 'COMPLETE':
            raise ValueError(f'incomplete job: {job}')
        folder = path / job['directory']
        manifest = json.loads((folder / 'manifest.json').read_text())
        rows = []
        if len({r['repeat'] for r in manifest['runs']}) != len(manifest['runs']):
            raise ValueError('duplicate round identities')
        for record in manifest['runs']:
            if record['status'] != 'MEASURED':
                raise ValueError('unmeasured run cannot be plotted')
            doc = json.loads((folder / record['file']).read_text())
            if doc['build_type'] != 'Release' or doc['sanitizer'] != 'none' or len(doc['results']) != 1:
                raise ValueError('requires one unsanitized Release result per process')
            row = doc['results'][0]
            if row['status'] != 'MEASURED' or not row['raw_samples']:
                raise ValueError('missing measured samples')
            expected_kind = 'single_event' if job['mode'] == 'latency' else 'replay_mean'
            if row['sample_kind'] != expected_kind or row['benchmark'] != 'order_book_' + job['mode']:
                raise ValueError('incompatible measurement units/boundaries')
            m, c = row['metrics'], doc['config']
            # Source/binary hashes intentionally differ after optimization. The
            # toolchain, stream, sample count, CPU and measurement settings may not.
            signature = (doc['compiler'], doc['build_type'], doc['sanitizer'], row['unit'], row['sample_kind'],
                         c['seed'], c['threads'], tuple(c['cpus']), c['warmup'], c['iterations'], c['duration_seconds'],
                         m['workload_version'], m['trace_hash_hi'], m['trace_hash_lo'], m['trace_events'],
                         m['initial_orders'], m['capacity'], row['sample_count'], row['operations_per_sample'])
            # Preserve build flags without comparing repository paths or hashes.
            cache = (folder / 'CMakeCache.txt').read_text()
            flags = tuple(line for line in cache.splitlines() if line.startswith(
                ('CMAKE_CXX_FLAGS:', 'CMAKE_CXX_FLAGS_RELEASE:', 'LAB_NATIVE:', 'LAB_LTO:', 'LAB_SANITIZER:')))
            signature += (flags,)
            environment = (folder / 'environment.txt').read_text()
            sections = environment.split('\n## ')
            stable_sections = tuple(section.strip() for section in sections if section.startswith(
                ('uname -a\n', 'lscpu -e=', 'getconf GNU_LIBC_VERSION\n',
                 'cat /sys/devices/system/cpu/smt/active\n', 'cat /sys/devices/system/cpu/cpufreq/boost\n',
                 'cat /proc/sys/kernel/randomize_va_space\n')))
            cpu_model = tuple(line for line in environment.splitlines() if line.startswith('Model name:'))
            governors = tuple(line for line in environment.splitlines() if '/scaling_governor:' in line)
            signature += (stable_sections, cpu_model, governors, m.get('pmu_measured'),
                          tuple(m.get(key) for key in ['accepted_events', 'not_found_events', 'rejected_events', 'trades',
                                                     'matched_quantity', 'unfilled_market_quantity', 'final_orders_per_replay']))
            rows.append({'signature': signature, 'row': row, 'file': str(folder / record['file'])})
        if len(rows) < 3 or len({entry['signature'] for entry in rows}) != 1:
            raise ValueError('need >=3 independent compatible rounds')
        key = (job['size'], job['mode'])
        if key in groups:
            raise ValueError('duplicate campaign size/mode')
        groups[key] = rows
    sizes = sorted({n for n, _ in groups})
    if set(groups) != {(n, mode) for n in sizes for mode in ['latency', 'throughput']}:
        raise ValueError('each size needs latency and throughput')
    for n in sizes:
        left, right = [groups[(n, mode)][0]['row']['metrics'] for mode in ['latency', 'throughput']]
        if any(left[key] != right[key] for key in ['trace_hash_hi', 'trace_hash_lo', 'trace_events', 'capacity']):
            raise ValueError('latency and throughput must replay the same stream')
    return campaign['label'], groups


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--campaign', type=Path, action='append', required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    campaigns = [load_campaign(path) for path in a.campaign]
    baseline = campaigns[0][1]
    for _, groups in campaigns[1:]:
        if groups.keys() != baseline.keys():
            p.error('campaign sizes/modes differ')
        for key in baseline:
            if groups[key][0]['signature'] != baseline[key][0]['signature']:
                p.error(f'cannot compare different workloads/settings: {key}')
    if len({label for label, _ in campaigns}) != len(campaigns):
        p.error('use distinct campaign labels')
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    import numpy as np
    a.output.mkdir(parents=True, exist_ok=False)
    sizes = sorted({n for n, _ in baseline})
    plt.rcParams.update({'font.size': 10, 'axes.spines.top': False, 'axes.spines.right': False})
    fig, axes = plt.subplots(2, 2, figsize=(12, 8), layout='constrained')
    summary = {'campaigns': [], 'plot_script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    for label, groups in campaigns:
        records = []
        for axis, mode, key, title, unit, scale in [
            (axes[0, 0], 'latency', 'mean', 'Single-event mean', 'ns/event', 1),
            (axes[0, 1], 'latency', 'p99', 'Single-event p99', 'ns/event', 1),
            (axes[1, 0], 'latency', 'p999', 'Single-event p99.9', 'ns/event', 1),
            (axes[1, 1], 'throughput', 'ops_per_sec', 'Replay throughput', 'million events/s', 1e6)]:
            values = [[entry['row'][key] / scale for entry in groups[(n, mode)]] for n in sizes]
            center = [median(v) for v in values]
            errors = [[m - min(v) for m, v in zip(center, values)], [max(v) - m for m, v in zip(center, values)]]
            axis.errorbar(sizes, center, yerr=errors, marker='o', capsize=4, label=label)
            axis.set(xscale='log', xlabel='Initial resting orders (not steady-state depth)', ylabel=unit, title=title)
            axis.set_xticks(sizes, [str(n) for n in sizes]); axis.grid(alpha=.2); axis.legend(fontsize=8)
            for n, samples, value in zip(sizes, values, center):
                records.append({'size': n, 'mode': mode, 'metric': key, 'median': value, 'rounds': samples, 'unit': unit})
        summary['campaigns'].append({'label': label, 'metrics': records,
                                     'files': [entry['file'] for rows in groups.values() for entry in rows]})
    fig.suptitle('Order book: measured baseline' if len(campaigns) == 1 else 'Order book: measured before / after')
    fig.supxlabel('Independent processes: median and min–max. Closed-loop synthetic 60/25/10/5; no latency-overhead subtraction.', fontsize=9)
    for ext in ['png', 'svg']:
        fig.savefig(a.output / f'order-book-summary.{ext}', dpi=160)
    plt.close(fig)
    fig, axes = plt.subplots(1, len(sizes), figsize=(5 * len(sizes), 4), squeeze=False, layout='constrained')
    for index, n in enumerate(sizes):
        axis = axes[0, index]
        for color, (label, groups) in enumerate(campaigns):
            for repeat, entry in enumerate(groups[(n, 'latency')]):
                samples = entry['row']['raw_samples']
                values, first = np.unique(np.sort(samples), return_index=True)
                tail = (len(samples) - first) / len(samples)
                axis.step(values, tail, where='pre', color=f'C{color}', alpha=.55,
                          label=label if repeat == 0 else None)
        axis.set(xscale='log', yscale='log', xlabel='Single-event latency (ns)', ylabel='Empirical P(latency >= x)', title=f'Initial orders: {n}')
        axis.grid(alpha=.2); axis.legend(fontsize=8)
    fig.suptitle('Each line is one process; tail resolution is limited by event count')
    for ext in ['png', 'svg']:
        fig.savefig(a.output / f'order-book-tail.{ext}', dpi=160)
    plt.close(fig)
    (a.output / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(a.output / 'order-book-summary.png')


if __name__ == '__main__':
    main()
