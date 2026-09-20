#!/usr/bin/env python3
"""Sequential loopback campaigns. Build/test before launching; no concurrent compilation."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
p.add_argument('--output',type=Path,required=True)
p.add_argument('--cpu',default='0,1')
p.add_argument('--binary',type=Path,default=ROOT/'build/release/lab_bench')
a=p.parse_args()
out=a.output.resolve(); out.mkdir(parents=True,exist_ok=False)
jobs=[('baseline',['--suite','network','--size','64','--batch','16','--iterations','10000','--warmup','200','--perf'])]
for size in [512,4096]:
    jobs.append((f'payload-{size}',['--benchmark','network_rtt','--size',str(size),'--batch','16','--iterations','1000','--warmup','100']))
for batch in [1,8,64]:
    for variant in ['tcp_unbatched','tcp_batched','udp_batch']:
        jobs.append((f'window-{batch}-{variant}',['--benchmark','network_rtt','--variant',variant,'--size','64',
                    '--batch',str(batch),'--iterations','1000','--warmup','100']))
# Hold both endpoint threads on the client CPU to expose busy retry scheduling cost.
jobs.append(('same-cpu',['--benchmark','network_io','--cpu',a.cpu.split(',')[0],
             '--size','64','--iterations','2000','--warmup','100']))
records=[]
try:
    for name,args in jobs:
        command=[sys.executable,str(ROOT/'tools/run_benchmark.py'),'--binary',str(a.binary.resolve()),
                 '--cpu',a.cpu,'--repeats','3','--timeout-ms','200','--output',str(out/name)]+args
        record={'name':name,'command':command,'status':'FAILED'}; records.append(record)
        subprocess.run(command,check=True)
        record['status']='COMPLETE'
finally:
    (out/'campaign.json').write_text(json.dumps(records,indent=2)+'\n')
