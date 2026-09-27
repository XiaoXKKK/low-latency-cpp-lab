"""Semantic/measurement contracts; no relative-speed assertions."""
import csv
import io
import json
import subprocess
import sys

exe = sys.argv[1]


def run(name, *args, output='json'):
    command = [exe, '--benchmark', name, '--iterations', '3', '--warmup', '2',
               '--size', '16', '--batch', '7', '--format', output, *args]
    return subprocess.run(command, check=True, capture_output=True, text=True, timeout=30).stdout


doc = json.loads(run('containers'))
assert len(doc['results']) == 24
for row in doc['results']:
    operation = row['variant'].rsplit('_', 1)[1]
    assert row['status'] == 'MEASURED' and row['sample_count'] == 3
    assert row['operations_per_sample'] == (16 if operation == 'iterate' else 7)
    assert row['sample_kind'] == 'batch_mean' and row['ops_per_sec'] > 0
    assert row['metrics']['final_records'] == {'insert': 23, 'erase': 9}.get(operation, 16)
    if operation == 'lookup':
        assert abs(row['metrics']['hit_fraction'] - 4 / 7) < 1e-6
assert all(row['operations_per_sample'] == 1 for row in json.loads(run('containers', '--size', '1', '--batch', '7'))['results'] if row['variant'].endswith('_erase'))
for n in ['1', '31', '100000']:
    rows = json.loads(run('data_layout', '--size', n))['results']
    assert len(rows) == 2 and rows[0]['metrics']['checksum'] == rows[1]['metrics']['checksum']
    assert all(row['operations_per_sample'] == int(n) and row['sample_kind'] == 'full_pass_mean' for row in rows)
    assert rows[0]['metrics']['logical_storage_bytes'] >= rows[1]['metrics']['logical_storage_bytes']
for name, count in [('containers', 24), ('data_layout', 2)]:
    rows = list(csv.DictReader(io.StringIO(run(name, output='csv'))))
    assert len(rows) == count and all(None not in row for row in rows)
    rows = json.loads(run(name, '--duration', '0.000000001'))['results']
    assert all(row['sample_count'] == 1 for row in rows)
for name, args in [('containers', ['--size', '100001']), ('containers', ['--batch', '4097']),
                   ('containers', ['--threads', '2']), ('data_layout', ['--threads', '2']),
                   ('data_layout', ['--size', '1000001']), ('containers', ['--variant', 'missing'])]:
    result = subprocess.run([exe, '--benchmark', name, *args], capture_output=True, timeout=10)
    assert result.returncode != 0
print('containers/layout: all variants, operation counts, exact layout checksum, bounds, JSON/CSV/duration PASS')
