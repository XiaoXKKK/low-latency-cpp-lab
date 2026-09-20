#!/usr/bin/env python3
"""Reference full campaign for the first four Phase2 groups, sequential only."""
import argparse
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
p.add_argument('--output',type=Path,required=True)
p.add_argument('--cpus',default='0,1,2,3')
p.add_argument('--memory-nodes',default='0,1')
p.add_argument('--compiler-matrix',action='store_true')
a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
cpus=a.cpus.split(','); cpu=cpus[0]; pair=','.join(cpus[:2])
def run(label,*args):
    subprocess.run([sys.executable,str(ROOT/'tools/run_benchmark.py'),'--repeats','3','--cpu',pair,
                    '--output',str(out/label),*map(str,args)],check=True)
run('main','--suite','phase2','--iterations',100,'--warmup',10,'--batch',2048,'--perf')
run('arrival-tail','--benchmark','arrival_latency','--iterations',10000,'--warmup',100,'--interval-ns',20000,'--batch',4096)
run('arrival-overload','--benchmark','arrival_latency','--iterations',2000,'--warmup',100,'--interval-ns',1000,'--batch',4096)
run('wait-tail','--benchmark','wait_strategy','--iterations',5000,'--warmup',100,'--interval-ns',100000)
run('streaming-8m','--benchmark','cache_patterns','--variant','streaming','--size',8388608)
for stride in [1,2,4,8,16,32,64,128]:
    run(f'stride-{stride}','--benchmark','cache_patterns','--variant','stride','--size',8388608,'--stride',stride)
run('indexed-8m','--benchmark','cache_patterns','--variant','indexed','--size',8388608)
for distance in [0,4,16,64,256]:
    run(f'prefetch-{distance}','--benchmark','cache_patterns','--variant','prefetch','--size',8388608,'--distance',distance)
for batch in [16,256,4096]:
    run(f'allocator-{batch}','--benchmark','allocator_batch','--batch',batch)
for readers in [0,50,90,100]:
    run(f'rw-{readers}','--benchmark','rw_locks','--threads',len(cpus),'--cpu',a.cpus,'--read-percent',readers,'--critical',32,'--iterations',50)
for critical in [0,32,256]:
    run(f'locks-{critical}','--benchmark','locks','--threads',len(cpus),'--cpu',a.cpus,'--critical',critical,'--batch',2048,'--iterations',50)
run('locks-oversubscribed','--benchmark','locks','--threads',2,'--cpu',cpu,'--batch',16,'--iterations',10,'--warmup',2)
for node in a.memory_nodes.split(','):
    run(f'numa-{node}','--benchmark','numa_access','--cpu',cpu,'--memory-node',node,'--size',67108864,'--iterations',3,'--warmup',1)
if a.compiler_matrix:
    subprocess.run([sys.executable,str(ROOT/'tools/compiler_matrix.py'),'--output',str(out/'compilers'),'--cpu',cpu,'--compilers','/usr/bin/g++','clang++'],check=True)
