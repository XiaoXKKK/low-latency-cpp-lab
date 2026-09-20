import csv
import io
import json
import math
import subprocess
import sys
exe = sys.argv[1]
for name in ['timer', 'memory_access', 'false_sharing', 'branch', 'allocation', 'locks', 'spsc', 'affinity']:
    threads = '2' if name in ['false_sharing', 'locks', 'spsc'] else '1'
    base = [exe, '--benchmark', name, '--threads', threads, '--iterations', '3', '--warmup', '1', '--batch', '64']
    p = subprocess.run(base + ['--format', 'json'], text=True, capture_output=True, timeout=30, check=True)
    doc = json.loads(p.stdout)
    assert doc['results'], name
    for row in doc['results']:
        assert row['sample_count'] == len(row['raw_samples'])
        assert row['min'] <= row['p50'] <= row['p90'] <= row['p95'] <= row['p99'] <= row['p999'] <= row['max']
        assert math.isfinite(row['mean']) and row['mean'] >= 0
        assert (row['ops_per_sec'] is None) == (row['mode'] == 'latency')
    p = subprocess.run(base + ['--format', 'csv'], text=True, capture_output=True, timeout=30, check=True)
    rows = list(csv.DictReader(io.StringIO(p.stdout)))
    assert len(rows) == len(doc['results']) and None not in rows[0]
p = subprocess.run([exe, '--benchmark', 'timer', '--iterations', '2', '--format', 'table'], text=True, capture_output=True, check=True)
lines = p.stdout.splitlines()
assert all(len(line.split(' | ')) == 22 for line in lines), lines
for args in [['--iterations','0'], ['--warmup','-1'], ['--duration','nan'], ['--duration','-1'],
             ['--cpu','999999'], ['--threads','0'], ['--format','xml'], ['--variant','missing'], ['--unknown','1'],
             ['--benchmark','missing'], ['--iterations','1x'], ['--cpu','0,'], ['--iterations']]:
    p = subprocess.run([exe]+args, capture_output=True, timeout=10)
    assert p.returncode != 0, args
p = subprocess.run([exe, '--benchmark', 'memory_access', '--variant', 'sequential', '--size', '65536', '--batch', '64', '--iterations', '2', '--warmup', '0', '--format', 'json'], text=True, capture_output=True, check=True)
assert json.loads(p.stdout)['results'][0]['operations_per_sample'] == 65536 // 4
p = subprocess.run([exe, '--benchmark', 'timer', '--variant', 'steady_clock', '--iterations', '100', '--warmup', '0', '--duration', '0.000000001', '--format', 'json'], text=True, capture_output=True, check=True)
assert json.loads(p.stdout)['results'][0]['sample_count'] == 1
print('8 benchmark schemas, CSV/table, invalid CLI, duration and working-set coverage: PASS')
