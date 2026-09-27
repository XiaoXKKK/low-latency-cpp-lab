#!/usr/bin/env python3
"""Plot only measured runs; refuse different traces or incompatible settings."""
import argparse
import bisect
import hashlib
import json
import math
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
            rows.append({'signature': signature, 'row': row, 'file': str(folder / record['file']), 'repeat': record['repeat']})
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


def histogram_data(series, bin_width=5.0, x_max=None, view_percentile=99.5, stat='count'):
    """Full-sample medians, shared edges, explicit overflow; no sample replication.

    series contains (label, selected independent-run entries) for ONE workload.
    This helper uses only stdlib so counting/tail semantics need no plot runtime.
    """
    if not math.isfinite(bin_width) or bin_width <= 0:
        raise ValueError('bin width must be finite and positive')
    if x_max is not None and (not math.isfinite(x_max) or x_max <= 0):
        raise ValueError('x max must be finite and positive')
    if not 0 < view_percentile <= 100 or stat not in ('count', 'probability'):
        raise ValueError('invalid viewport percentile or histogram statistic')
    if not series:
        raise ValueError('histogram requires measured series')
    prepared, quantiles = [], []
    for label, entries in series:
        values, run_medians, repeats = [], [], []
        for entry in entries:
            row = entry['row']
            samples = row['raw_samples']
            if row['sample_kind'] != 'single_event' or row['unit'] != 'ns/event' or row['operations_per_sample'] != 1:
                raise ValueError('histograms require single-event ns/event samples, never replay means')
            if not samples or len(samples) != row['sample_count']:
                raise ValueError('incomplete raw samples')
            if any(not math.isfinite(value) or value < 0 for value in samples):
                raise ValueError('latencies must be finite and nonnegative')
            values.extend(samples); run_medians.append(median(samples)); repeats.append(entry['repeat'])
        if not values or len(repeats) != len(set(repeats)):
            raise ValueError('empty series or duplicate rounds')
        values.sort()
        position = (len(values) - 1) * view_percentile / 100
        lo, hi = math.floor(position), math.ceil(position)
        quantiles.append(values[lo] + (values[hi] - values[lo]) * (position - lo))
        prepared.append({'label': label, 'values': values, 'sample_count': len(values),
                         'median_ns': median(values), 'round_medians_ns': run_medians, 'round_ids': repeats,
                         'max_ns': values[-1], 'files': [entry['file'] for entry in entries]})
    if stat == 'count' and len({row['sample_count'] for row in prepared}) != 1:
        raise ValueError('frequency overlays require equal sample counts; select equal rounds or use --hist-stat probability')
    upper = x_max if x_max is not None else max(quantiles)
    ratio = max(upper, bin_width) / bin_width
    if not math.isfinite(ratio) or ratio > 20000:
        raise ValueError('more than 20000 bins; increase --bin-width-ns or reduce --x-max-ns')
    bins = max(1, math.ceil(ratio))
    edges = [i * bin_width for i in range(bins + 1)]
    for row in prepared:
        values = row.pop('values')
        counts, overflow = [0] * bins, 0
        for value in values:
            if value > edges[-1]:
                overflow += 1
            else:
                counts[min(bisect.bisect_right(edges, value) - 1, bins - 1)] += 1
        row.update(counts=counts, overflow_count=overflow, overflow_fraction=overflow / len(values),
                   heights=counts if stat == 'count' else [100 * count / len(values) for count in counts])
    return {'edges_ns': edges, 'bin_width_ns': bin_width, 'x_max_ns': edges[-1],
            'view_percentile': view_percentile if x_max is None else None, 'stat': stat, 'series': prepared}


def draw_histograms(campaigns, sizes, args, plt):
    palette = ['#e4ce39', '#70b2df', '#db8574', '#66ad88', '#ae8bc3']
    artifact = {'plot_script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                'round_selection': args.hist_round, 'histograms': []}
    for n in sizes:
        series = []
        for label, groups in campaigns:
            entries = sorted(groups[(n, 'latency')], key=lambda entry: entry['repeat'])
            if args.hist_round != 'all':
                entries = [entry for entry in entries if entry['repeat'] == int(args.hist_round)]
                if not entries:
                    raise ValueError(f'round {args.hist_round} not found for {label}, N={n}')
            series.append((label, entries))
        data = histogram_data(series, args.bin_width_ns, args.x_max_ns, args.view_percentile, args.hist_stat)
        data['initial_orders'] = n
        fig, axis = plt.subplots(figsize=(10, 6), layout='constrained')
        title = 'OrderBookMap Latencies — Baseline' if len(series) == 1 else 'OrderBook Latencies — Comparison'
        fig.suptitle(title, fontfamily='DejaVu Serif', fontsize=22, color='#20354b')
        selection = f'run {args.hist_round}' if args.hist_round != 'all' else 'runs pooled for display'
        axis.set_title(f'OrderBook Latency Distribution · Initial orders: {n} · {selection}', fontsize=12)
        notes = [f'{args.bin_width_ns:g} ns common bins; medians use ALL samples, including the tail outside this view.']
        for i, row in enumerate(data['series']):
            color = palette[i % len(palette)]
            axis.stairs(row['heights'], data['edges_ns'], fill=True, alpha=.58, color=color,
                        linewidth=.5, label=row['label'])
            outside = ' (outside view)' if row['median_ns'] > data['x_max_ns'] else ''
            axis.axvline(row['median_ns'], color=color, linestyle='--', linewidth=1.6,
                         label=f"Median: {row['median_ns']:.1f} ns{outside}")
            notes.append(f"{row['label']}: n={row['sample_count']:,}; outside view={row['overflow_count']:,} "
                         f"({100 * row['overflow_fraction']:.2f}%); max={row['max_ns']:,.0f} ns")
        axis.set(xlim=(0, data['x_max_ns']), ylim=(0, None), xlabel='Latency (ns)',
                 ylabel='Frequency (events)' if args.hist_stat == 'count' else 'Probability per bin (%)')
        axis.grid(axis='y', alpha=.15); axis.set_axisbelow(True)
        axis.legend(loc='upper right', fontsize=9, framealpha=.95)
        fig.supxlabel('\n'.join(notes), fontsize=8)
        for ext in ['png', 'svg']:
            fig.savefig(args.output / f'order-book-histogram-n{n}.{ext}', dpi=160)
        plt.close(fig)
        artifact['histograms'].append(data)
    (args.output / 'histograms.json').write_text(json.dumps(artifact, indent=2) + '\n')


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--campaign', type=Path, action='append', required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--style', choices=['all', 'histogram', 'overview'], default='all')
    p.add_argument('--bin-width-ns', type=float, default=5)
    p.add_argument('--x-max-ns', type=float, help='viewport only, rounded up to a whole bin; tails remain in statistics')
    p.add_argument('--view-percentile', type=float, default=99.5, help='automatic viewport percentile if x max omitted')
    p.add_argument('--hist-round', default='0', help='independent repeat ID (default 0), or all to pool for display')
    p.add_argument('--hist-stat', choices=['count', 'probability'], default='count')
    a = p.parse_args()
    if a.hist_round != 'all' and (not a.hist_round.isdigit() or int(a.hist_round) < 0):
        p.error('hist-round must be a nonnegative repeat ID or all')
    if (not math.isfinite(a.bin_width_ns) or a.bin_width_ns <= 0 or not 0 < a.view_percentile <= 100 or
        (a.x_max_ns is not None and (not math.isfinite(a.x_max_ns) or a.x_max_ns <= 0))):
        p.error('positive finite bin width/x max and percentile in (0,100] required')
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
    if a.style in ('all', 'histogram'):
        draw_histograms(campaigns, sizes, a, plt)
        print(a.output / f'order-book-histogram-n{sizes[0]}.png')
    if a.style == 'histogram':
        return
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
