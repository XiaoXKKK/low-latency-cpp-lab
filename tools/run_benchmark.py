#!/usr/bin/env python3
"""Run independent, randomized rounds with honest unavailable states and units."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
from statistics import median
import run_perf

ROOT=Path(__file__).resolve().parents[1]
VARIANTS={
    'timer':['empty_loop','steady_clock','clock_gettime','rdtsc_raw','lfence_rdtsc','rdtscp_lfence'],
    'memory_access':['sequential','random'], 'false_sharing':['packed','padded'],
    'branch':['sorted_branch','random_branch','sorted_branchless','random_branchless'],
    'allocation':['malloc_free','new_delete','pool','preallocated'],
    'locks':['mutex','spinlock','ticket'], 'spsc':['naive','padded'], 'affinity':['unpinned','pinned'],
    'tsc_interval':['empty','dependent_chain'], 'perf_interval':['empty','dependent_chain'],
    'cache_patterns':['streaming','stride','indexed','prefetch'],
    'allocator_batch':['malloc','new','arena','pmr_monotonic','pmr_pool','thread_local_arena','preallocated'],
    'allocation_handoff':['same_thread_free','cross_thread_free'],
    'page_behavior':['mapping_roundtrip','first_touch','warm_touch','thp','hugetlb'],
    'atomic_order':['relaxed_rmw','acquire_rmw','release_rmw','acq_rel_rmw','seq_cst_rmw','relaxed_store','release_store','seq_cst_store'],
    'rw_locks':['mutex','shared_mutex'], 'wait_strategy':['sleep','yield','spin','hybrid'],
    'arrival_latency':['closed_idle','open_idle','open_cpu','open_memory'],
    'numa_access':['bound_streaming','bound_random','first_touch_streaming','first_touch_random'],
}
NETWORK={'network_rtt':['tcp_default','tcp_nodelay','tcp_unbatched','tcp_batched','udp_rtt','udp_batch'],
         'network_io':['blocking','epoll_lt','epoll_et','busy_poll']}
PHASE1=list(VARIANTS)[:8]
PHASE2=['locks']+list(VARIANTS)[8:]
VARIANTS.update(NETWORK)
DEFAULT_SIZES={'network_rtt':64,'network_io':64,'page_behavior':2097152,'numa_access':8388608,'cache_patterns':4194304,'arrival_latency':8388608}
MULTITHREADED={'false_sharing','locks','spsc','allocation_handoff','rw_locks','network_rtt','network_io'}

def write_summary(output,grouped,skipped):
    lines=['# Measured benchmark summary','',f'Raw data and environment: `{output}`','',
           'Per-column median across independent rounds. Units are per row. Batch percentiles are not individual-operation tails.',
           'Small samples cannot establish p99.9; NOT MEASURED rows have no fabricated statistics.','',
           '| Experiment | Variant | Unit / sample kind | Rounds | mean | p50 | p99 | p999 | ops/sec |',
           '|---|---|---|---:|---:|---:|---:|---:|---:|']
    for (name,variant),rows in sorted(grouped.items()):
        cells=[f'{median(r[key] for r in rows):.3f}' for key in ['mean','p50','p99','p999']]
        rate='—' if rows[0]['ops_per_sec'] is None else f'{median(r["ops_per_sec"] for r in rows):.0f}'
        lines.append(f'| {name} | {variant} | {rows[0]["unit"]} / {rows[0]["sample_kind"]} | {len(rows)} | '+' | '.join(cells)+f' | {rate} |')
    if skipped:
        lines+=['','## NOT MEASURED','']
        for key,reasons in sorted(skipped.items()):
            lines.append(f'- {key}: '+ '; '.join(sorted(reasons)))
    content='\n'.join(lines)+'\n'
    (output/'summary.md').write_text(content)
    processed=ROOT/'results/processed'; processed.mkdir(exist_ok=True)
    (processed/f'{output.name}.md').write_text(content)

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--binary',type=Path,default=ROOT/'build/release/lab_bench')
    p.add_argument('--benchmark',choices=list(VARIANTS)+['all'],default='all')
    p.add_argument('--suite',choices=['phase1','phase2','network','all'],default='all')
    p.add_argument('--variant')
    for key,default in [('timeout-ms',1000),('repeats',3),('iterations',100),('warmup',10),('batch',None),('seed',42),
                        ('locks',1),('critical',0),('stride',1),('distance',16),('interval-ns',100000),('read-percent',90),
                        ('memory-node',-1),('touch-cpu',-1),('background-cpu',-1)]:
        p.add_argument('--'+key,type=int,default=default)
    p.add_argument('--cpu',help='default first two allowed CPUs; verify topology')
    p.add_argument('--size',type=int,help='default is selected for each experiment')
    p.add_argument('--threads',type=int)
    p.add_argument('--duration',type=float,default=0)
    p.add_argument('--timeout',type=float,default=300)
    p.add_argument('--perf',action='store_true')
    p.add_argument('--output',type=Path)
    a=p.parse_args()
    if a.repeats<1 or a.iterations<1 or a.timeout<=0: p.error('positive repeats/iterations/timeout required')
    if a.variant and (a.benchmark=='all' or a.variant not in VARIANTS[a.benchmark]): p.error('variant requires matching single benchmark')
    binary=a.binary.resolve()
    output=(a.output or ROOT/'results/raw'/datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')).resolve()
    output.mkdir(parents=True,exist_ok=False)
    env=subprocess.run(['bash',str(ROOT/'tools/environment_report.sh')],capture_output=True,text=True,check=True).stdout
    (output/'environment.txt').write_text(env)
    cpus=a.cpu or ','.join(map(str,sorted(os.sched_getaffinity(0))[:2]))
    manifest={'binary':str(binary),'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),
              'arguments':{k:str(v) if isinstance(v,Path) else v for k,v in vars(a).items()},'runs':[]}
    sources=list(ROOT.glob('src/*.cpp'))+list(ROOT.glob('include/lab/*.hpp'))+list(ROOT.glob('benchmarks/**/*.cpp'))+[ROOT/'CMakeLists.txt']+list(ROOT.glob('tools/*.py'))
    manifest['source_sha256']={str(f.relative_to(ROOT)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sources}
    for filename in ['CMakeCache.txt','compile_commands.json']:
        if (binary.parent/filename).exists(): (output/filename).write_bytes((binary.parent/filename).read_bytes())
    suites={'phase1':PHASE1,'phase2':PHASE2,'network':list(NETWORK),'all':list(VARIANTS)}
    names=suites[a.suite] if a.benchmark=='all' else [a.benchmark]
    rng=random.Random(a.seed);grouped={};skipped={}
    try:
        for repeat in range(a.repeats):
            jobs=[(name,v) for name in names for v in ([a.variant] if a.variant else VARIANTS[name])];rng.shuffle(jobs)
            for name,variant in jobs:
                threads=a.threads if a.threads is not None else (2 if name in MULTITHREADED else 1)
                size=a.size if a.size is not None else DEFAULT_SIZES.get(name,32768)
                command=[str(binary),'--benchmark',name,'--variant',variant,'--threads',str(threads),'--cpu',cpus,'--size',str(size),'--format','json']
                command+=['--batch',str(a.batch if a.batch is not None else (16 if name in NETWORK else 4096))]
                for key in ['timeout_ms','iterations','warmup','seed','locks','critical','duration','stride','distance','interval_ns','read_percent','memory_node','touch_cpu','background_cpu']:
                    command+=['--'+key.replace('_','-'),str(getattr(a,key))]
                record={'repeat':repeat,'benchmark':name,'variant':variant,'command':command}
                filename=f'{name}-{variant}-{repeat}.json'
                try:
                    process=subprocess.run(command,capture_output=True,text=True,timeout=a.timeout)
                    record['returncode']=process.returncode
                    (output/f'{name}-{variant}-{repeat}.stderr').write_text(process.stderr)
                    if process.returncode:
                        legacy_skip=name=='timer' and variant in ['rdtsc_raw','lfence_rdtsc','rdtscp_lfence'] and 'unknown or unsupported variant' in process.stderr
                        record.update(status='NOT MEASURED' if legacy_skip else 'FAILED',reason=process.stderr.strip())
                        if not legacy_skip: raise RuntimeError(f'{name}/{variant}: {process.stderr}')
                        skipped.setdefault(f'{name}/{variant}',set()).add(record['reason'])
                    else:
                        doc=json.loads(process.stdout)
                        if doc['build_type']!='Release' or doc['sanitizer']!='none': raise RuntimeError('Requires unsanitized Release binary')
                        (output/filename).write_text(process.stdout)
                        measured=0
                        for row in doc['results']:
                            if row.get('status','MEASURED')=='MEASURED':
                                grouped.setdefault((name,row['variant']),[]).append(row);measured+=1
                            else: skipped.setdefault(f'{name}/{row["variant"]}',set()).add(row['notes'])
                        record.update(status='MEASURED' if measured==len(doc['results']) else 'PARTIALLY MEASURED' if measured else 'NOT MEASURED',file=filename)
                except Exception as error:
                    record.update(status='FAILED',reason=str(error));raise
                finally:
                    manifest['runs'].append(record)
                    (output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
                print(f'round {repeat+1}: {name}/{variant}: {record["status"]}',flush=True)
                if a.perf and repeat==0:
                    perf=run_perf.run(command,output/f'perf-{name}-{variant}')
                    if perf['status']=='FAILED': raise RuntimeError('perf child failed; inspect artifact')
    finally:
        (output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        write_summary(output,grouped,skipped)
    print(f'Saved {output}/summary.md')
if __name__=='__main__':main()
