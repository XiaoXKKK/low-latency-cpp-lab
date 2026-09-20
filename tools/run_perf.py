#!/usr/bin/env python3
"""Probe each event, retain unsupported/denied details, never invent counter values."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

EVENTS = ['task-clock', 'context-switches', 'cpu-migrations', 'page-faults', 'cycles',
          'instructions', 'branches', 'branch-misses', 'cache-references', 'cache-misses']

def run(command, output):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    report = {'command': command, 'status': 'NOT MEASURED', 'events': {}, 'ipc': None}
    env = dict(os.environ, LC_ALL='C')
    if not shutil.which('perf'):
        report['reason'] = 'perf executable unavailable'
    else:
        supported = []
        for event in EVENTS:
            probe = subprocess.run(['perf', 'stat', '-e', event, '--', 'true'], capture_output=True, text=True, env=env)
            ok = probe.returncode == 0 and '<not supported>' not in probe.stderr and '<not counted>' not in probe.stderr
            report['events'][event] = {'available': ok, 'status': 'NOT MEASURED', 'probe': probe.stderr.strip()}
            if ok:
                supported.append(event)
        if supported:
            raw = output / 'perf-stat.csv'
            process = subprocess.run(['perf', 'stat', '-x', ';', '-o', str(raw), '-e', ','.join(supported), '--'] + command,
                                     capture_output=True, text=True, env=env)
            (output / 'benchmark.stdout').write_text(process.stdout)
            (output / 'benchmark.stderr').write_text(process.stderr)
            report['returncode'] = process.returncode
            values = {}
            for event in supported:
                report['events'][event].update(status='NOT MEASURED', reason='No numeric count in actual run')
            for line in raw.read_text().splitlines():
                fields = line.split(';')
                if len(fields) >= 3:
                    try:
                        values[fields[2]] = float(fields[0].strip())
                        report['events'].setdefault(fields[2], {}).update(status='MEASURED', count=values[fields[2]], raw=line)
                    except ValueError:
                        if fields[2] in report['events']:
                            report['events'][fields[2]].update(status='NOT MEASURED', reason=fields[0].strip(), raw=line)
            report['status'] = ('FAILED' if process.returncode else
                                'NOT MEASURED' if not values else
                                'MEASURED' if all(e in values for e in EVENTS) else 'PARTIALLY MEASURED')
            report['counts'] = values
            if values.get('cycles', 0) > 0 and 'instructions' in values:
                report['ipc'] = values['instructions'] / values['cycles']
        else:
            report['reason'] = 'No permitted/supported perf events; inspect per-event probe errors.'
    (output / 'perf.json').write_text(json.dumps(report, indent=2) + '\n')
    return report

if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('command', nargs=argparse.REMAINDER)
    a = p.parse_args()
    command = a.command[1:] if a.command[:1] == ['--'] else a.command
    if not command:
        p.error('provide command after --')
    result = run(command, a.output)
    print(json.dumps(result, indent=2))
    raise SystemExit(1 if result['status'] == 'FAILED' else 0)
