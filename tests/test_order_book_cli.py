"""Semantic and sample-boundary contracts, never performance rankings."""
import csv
import io
import json
import subprocess
import sys

exe = sys.argv[1]


def run(name, *args, fmt='json'):
    command = [exe, '--benchmark', name, '--variant', 'map_list', '--size', '16',
               '--iterations', '300' if name.endswith('latency') else '3',
               '--batch', '300', '--warmup', '2', '--format', fmt, *args]
    return subprocess.run(command, capture_output=True, text=True, check=True, timeout=40).stdout


rows = {}
for name in ['order_book_latency', 'order_book_throughput']:
    doc = json.loads(run(name))
    assert len(doc['results']) == 1
    row = rows[name] = doc['results'][0]
    assert row['status'] == 'MEASURED' and row['variant'] == 'map_list' and row['unit'] == 'ns/event'
    assert row['min'] <= row['p50'] <= row['p99'] <= row['p999'] <= row['max']
    metrics = row['metrics']
    repeats = 1 if name.endswith('latency') else 3
    assert metrics['replayed_events'] == 300 * repeats
    assert metrics['add_events'] == 180 * repeats
    assert metrics['cancel_events'] == 75 * repeats
    assert metrics['modify_events'] == 30 * repeats
    assert metrics['match_events'] == 15 * repeats
    assert metrics['accepted_events'] + metrics['not_found_events'] + metrics['rejected_events'] == metrics['replayed_events']
    if repeats == 1:
        assert row['sample_kind'] == 'single_event' and row['operations_per_sample'] == 1
        assert row['sample_count'] == 300 and row['ops_per_sec'] is None
    else:
        assert row['sample_kind'] == 'replay_mean' and row['operations_per_sample'] == 300
        assert row['sample_count'] == 3 and row['ops_per_sec'] > 0
        assert metrics['pmu_measured'] in [0, 1]
        if not metrics['pmu_measured']:
            assert 'PMU NOT MEASURED' in row['notes'] and 'cycles_per_event' not in metrics
    table = list(csv.DictReader(io.StringIO(run(name, fmt='csv'))))
    assert len(table) == 1 and None not in table[0]
    limited = json.loads(run(name, '--duration', '0.000000001'))['results'][0]
    assert limited['sample_count'] == 1
    assert limited['metrics']['replayed_events'] == (1 if repeats == 1 else 300)
    tiny = json.loads(run(name, '--size', '1', '--iterations', '1', '--batch', '1', '--warmup', '3'))['results'][0]
    assert tiny['sample_count'] == 1
for key in ['trace_hash_hi', 'trace_hash_lo', 'capacity', 'final_orders_per_replay', 'peak_orders_per_replay']:
    assert rows['order_book_latency']['metrics'][key] == rows['order_book_throughput']['metrics'][key]
for name, args in [('order_book_latency', ['--size', '10001']), ('order_book_latency', ['--iterations', '100001']),
                   ('order_book_throughput', ['--batch', '100001']), ('order_book_latency', ['--threads', '2']),
                   ('order_book_latency', ['--variant', 'imaginary_optimization'])]:
    p = subprocess.run([exe, '--benchmark', name, *args], capture_output=True, timeout=10)
    assert p.returncode != 0
print('OrderBook CLI: ratios, same-stream latency/throughput, PMU fallback, JSON/CSV, duration/bounds PASS')
