#!/usr/bin/env python3
"""Build and measure one compiler/profile at a time; never compile during measurement."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]
def compiler_id(resolved):
    return Path(resolved).name.replace('+','p')+'-'+hashlib.sha256(str(Path(resolved).resolve()).encode()).hexdigest()[:8]
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--cpu',default='0')
    parser.add_argument('--compilers',nargs='+',default=['g++','clang++'])
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    cmake=shutil.which('cmake') or str(ROOT/'.tools/bin/cmake')
    profiles=[('O0','-O0',False,False),('O1','-O1',False,False),('O2','-O2',False,False),
              ('O3','-O3',False,False),('O3-native','-O3',True,False),('O3-lto','-O3',False,True)]
    report=[]
    try:
        for compiler in args.compilers:
            resolved=shutil.which(compiler)
            if not resolved:
                report.append({'compiler':compiler,'status':'NOT MEASURED','reason':'compiler unavailable'});continue
            for label,opt,native,lto in profiles:
                tag=compiler_id(resolved)+'-'+label;build=ROOT/'build/compiler-matrix'/tag
                command=[cmake,'-S',str(ROOT),'-B',str(build),'-DCMAKE_BUILD_TYPE=Release',f'-DCMAKE_CXX_COMPILER={resolved}',
                         f'-DCMAKE_CXX_FLAGS_RELEASE={opt} -DNDEBUG','-DLAB_SANITIZER=none',f'-DLAB_NATIVE={"ON" if native else "OFF"}',f'-DLAB_LTO={"ON" if lto else "OFF"}']
                log_file=out/f'{tag}-build.log'
                item={'compiler':compiler,'profile':label,'tag':tag,'configure':command,'status':'FAILED','log':str(log_file)};report.append(item)
                stage='configure'
                try:
                    with log_file.open('w') as log:
                        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
                        stage='build'
                        subprocess.run([cmake,'--build',str(build),'-j','4'],stdout=log,stderr=subprocess.STDOUT,check=True)
                        stage='test'
                        subprocess.run([str(Path(cmake).parent/'ctest'),'--test-dir',str(build),'--output-on-failure'],stdout=log,stderr=subprocess.STDOUT,check=True)
                    stage='assembly'
                    with (out/f'{tag}-assembly.txt').open('w') as assembly:
                        subprocess.run(['objdump','-d','-C',str(build/'lab_bench')],stdout=assembly,check=True)
                    stage='benchmark'
                    for benchmark,variant in [('cache_patterns','streaming'),('branch','random_branch'),('perf_interval','dependent_chain')]:
                        subprocess.run([sys.executable,str(ROOT/'tools/run_benchmark.py'),'--binary',str(build/'lab_bench'),
                            '--benchmark',benchmark,'--variant',variant,'--repeats','3','--iterations','100','--warmup','10',
                            '--size','1048576','--batch','65536','--cpu',args.cpu,'--output',str(out/f'{tag}-{benchmark}')],check=True)
                    item['status']='MEASURED';print(tag,'complete',flush=True)
                except FileNotFoundError as error:
                    item.update(status="NOT MEASURED",reason=f"{stage} tool unavailable: {error}")
                    print(tag,item["status"],item["reason"],flush=True)
                except subprocess.CalledProcessError as error:
                    optional_ipo=stage=='configure' and lto and 'IPO unavailable' in log_file.read_text()
                    item.update(status='NOT MEASURED' if optional_ipo else 'FAILED',reason=f'{stage}: {error}')
                    print(tag,item['status'],item['reason'],flush=True)
                    # Continue independent profiles; a real build/test failure still makes the final exit nonzero.
    finally:
        (out/'compiler-matrix.json').write_text(json.dumps(report,indent=2)+'\n')
    return 1 if any(item['status']=='FAILED' for item in report) else 0
if __name__=='__main__':raise SystemExit(main())
