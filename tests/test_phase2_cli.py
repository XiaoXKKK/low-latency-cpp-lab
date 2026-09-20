import csv
import io
import json
import subprocess
import sys
exe=sys.argv[1]
benchmarks=['tsc_interval','perf_interval','cache_patterns','allocator_batch','allocation_handoff',
            'page_behavior','atomic_order','rw_locks','wait_strategy','arrival_latency','numa_access']
for benchmark in benchmarks:
    threads=2 if benchmark in ['allocation_handoff','rw_locks'] else 1
    command=[exe,'--benchmark',benchmark,'--threads',str(threads),'--size','2097152','--iterations','3',
             '--warmup','1','--batch','64','--interval-ns','10000']
    p=subprocess.run(command+['--format','json'],capture_output=True,text=True,check=True,timeout=60)
    doc=json.loads(p.stdout)
    assert doc['schema_version']==2 and doc['results']
    for row in doc['results']:
        if row['status']=='NOT MEASURED':
            assert not row['raw_samples'] and row['mean'] is None and row['ops_per_sec'] is None and row['notes']
            assert benchmark in ['perf_interval','tsc_interval','page_behavior','numa_access'],row
        else:
            assert row['sample_count']==len(row['raw_samples']) and row['sample_count']>0
            assert 0<=row['min']<=row['p50']<=row['p99']<=row['p999']<=row['max']
    p=subprocess.run(command+['--format','csv'],capture_output=True,text=True,check=True,timeout=60)
    rows=list(csv.DictReader(io.StringIO(p.stdout)))
    assert len(rows)==len(doc['results'])
    assert all(None not in row and row['status'] for row in rows)
for args in [['--stride','0'],['--read-percent','101'],['--interval-ns','0'],['--background-cpu','9999']]:
    assert subprocess.run([exe]+args,capture_output=True).returncode!=0
# Overload keeps the planned timeline rather than rebasing after every completion.
p=subprocess.run([exe,'--benchmark','arrival_latency','--variant','open_idle','--iterations','20',
                  '--warmup','1','--interval-ns','1','--batch','4096','--format','json'],capture_output=True,text=True,check=True)
response,service=json.loads(p.stdout)['results']
assert response['metrics']['max_scheduled_backlog']>0
assert all(r>=s for r,s in zip(response['raw_samples'],service['raw_samples']))
print('Phase2 variants, explicit unavailable rows, CSV/schema, open-schedule overload: PASS')
