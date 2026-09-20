"""Real loopback protocol/CLI integration, independent of performance thresholds."""
import csv
import io
import json
import os
import subprocess
import sys
exe=sys.argv[1]
cpus=','.join(map(str,sorted(os.sched_getaffinity(0))[:2]))
for benchmark,count in [('network_rtt',6),('network_io',4)]:
    command=[exe,'--benchmark',benchmark,'--threads','2','--cpu',cpus,
             '--size','128','--batch','4','--iterations','8','--warmup','2','--timeout-ms','500']
    doc=json.loads(subprocess.run(command+['--format','json'],check=True,capture_output=True,text=True,timeout=40).stdout)
    assert len(doc['results'])==count
    assert doc['config']['timeout_ms']==500
    for row in doc['results']:
        assert row['status']=='MEASURED' and row['threads']==2
        metrics=row['metrics']
        assert metrics['attempted_messages']==metrics['completed_messages']+metrics['unanswered_messages']
        assert metrics['process_cpu_cores']>=0
        assert metrics['duplicate_replies']==metrics['stale_replies']==0
        assert row['sample_count']>0 and row['min']<=row['p99']<=row['max']
        if row['mode']=='latency':
            assert row['unit']=='ns/roundtrip' and row['ops_per_sec'] is None
        else:
            assert row['operations_per_sample']==4 and row['ops_per_sec']>0
    rows=list(csv.DictReader(io.StringIO(subprocess.run(command+['--format','csv'],check=True,capture_output=True,text=True,timeout=40).stdout)))
    assert len(rows)==count and all(None not in row for row in rows)
for args in [['--threads','1'],['--threads','2','--size','7'],['--threads','2','--size','65508'],
             ['--threads','2','--batch','257','--variant','udp_batch'],['--timeout-ms','0'],['--timeout-ms','60001']]:
    p=subprocess.run([exe,'--benchmark','network_rtt']+args,capture_output=True,timeout=10)
    assert p.returncode!=0,args
print('Loopback TCP/UDP, LT/ET/busy, JSON/CSV, loss accounting and bounds PASS')
