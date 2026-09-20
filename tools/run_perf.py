#!/usr/bin/env python3
"""Probe each event, retain unsupported/denied details, never invent counter values."""
import argparse
import json
import math
import os
from pathlib import Path
import shutil
import subprocess

EVENTS = ['task-clock', 'context-switches', 'cpu-migrations', 'page-faults', 'cycles',
          'instructions', 'branches', 'branch-misses', 'cache-references', 'cache-misses']

def run(command, output):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    report = {'command': command, 'status': 'NOT MEASURED', 'events': {}, 'ipc': None,
              'scaled_ipc': None, 'ipc_note': 'Requires a fully scheduled cycles/instructions group.'}
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
            # Schedule the IPC pair together; other counters may multiplex independently.
            event_args = [event for event in supported if event not in ('cycles', 'instructions')]
            if all(event in supported for event in ('cycles', 'instructions')):
                event_args.append('{cycles,instructions}')
            else:
                event_args.extend(event for event in ('cycles', 'instructions') if event in supported)
            process = subprocess.run(['perf', 'stat', '-x', ';', '-o', str(raw), '-e', ','.join(event_args), '--'] + command,
                                     capture_output=True, text=True, env=env)
            (output / 'benchmark.stdout').write_text(process.stdout)
            (output / 'benchmark.stderr').write_text(process.stderr)
            report['returncode'] = process.returncode
            values = {}
            for event in supported:
                report['events'][event].update(status='NOT MEASURED', reason='No numeric count in actual run')
            for line in (raw.read_text() if raw.exists() else '').splitlines():
                fields = line.split(';')
                if len(fields) >= 3:
                    try:
                        count = float(fields[0].strip())
                        if not math.isfinite(count) or count < 0:
                            raise ValueError('invalid counter')
                        values[fields[2]] = count
                        event = report['events'].setdefault(fields[2], {})
                        event.pop('reason', None)
                        event.update(status='MEASURED', count=count, unit=fields[1], raw=line,
                                     running_ns=None, running_percent=None, multiplexed=None)
                        if len(fields) >= 5:
                            try:
                                runtime, percent = float(fields[3]), float(fields[4])
                                if math.isfinite(runtime) and runtime >= 0 and math.isfinite(percent) and 0 <= percent <= 100:
                                    event.update(running_ns=runtime, running_percent=percent,
                                                 multiplexed=percent < 100)
                            except ValueError:
                                pass
                    except ValueError:
                        if fields[2] in report['events']:
                            report['events'][fields[2]].update(status='NOT MEASURED', reason=fields[0].strip(), raw=line)
            report['status'] = ('FAILED' if process.returncode else
                                'NOT MEASURED' if not values else
                                'MEASURED' if all(e in values for e in EVENTS) else 'PARTIALLY MEASURED')
            report['counts'] = values
            if process.returncode == 0 and values.get('cycles', 0) > 0 and 'instructions' in values:
                ratio = values['instructions'] / values['cycles']
                if all(report['events'][event].get('running_percent') == 100 for event in ('cycles', 'instructions')):
                    report.update(ipc=ratio, ipc_note='Whole-process grouped IPC; perf reports 100% running (rounded). Includes startup and warmup.')
                else:
                    report.update(scaled_ipc=ratio, ipc_note='Scaled grouped count ratio only; multiplexed or unknown running coverage. Not exact interval IPC.')
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
