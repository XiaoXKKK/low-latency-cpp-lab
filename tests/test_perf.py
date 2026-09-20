"""Counter probe success must not hide combined-run counter failure."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
spec = importlib.util.spec_from_file_location('run_perf', Path(__file__).resolve().parents[1]/'tools/run_perf.py')
perf = importlib.util.module_from_spec(spec)
spec.loader.exec_module(perf)
class PerfTest(unittest.TestCase):
    def check_counts(self, raw, expected):
        def fake(command, **kwargs):
            if '-o' in command:
                Path(command[command.index('-o')+1]).write_text(raw)
            return subprocess.CompletedProcess(command, 0, '', '')
        with tempfile.TemporaryDirectory() as temp, patch.object(perf.shutil, 'which', return_value='/usr/bin/perf'), patch.object(perf.subprocess, 'run', side_effect=fake):
            result = perf.run(['true'], temp)
            self.assertEqual(result['status'], expected)
            self.assertEqual(result['events']['cycles']['status'], 'NOT MEASURED')
            self.assertIsNone(result['ipc'])
            self.assertEqual(json.loads((Path(temp)/'perf.json').read_text())['status'], expected)
    def test_empty(self):
        self.check_counts('<not counted>;;cycles;0;0\n', 'NOT MEASURED')
    def test_partial(self):
        self.check_counts('<not supported>;;cycles;0;0\n100;;instructions;1;100\n', 'PARTIALLY MEASURED')
    def test_scheduling_and_success_reason(self):
        for coverage in ('100.00', '75.00', ''):
            def fake(command, **kwargs):
                if '-o' in command:
                    self.assertIn('{cycles,instructions}', command[command.index('-e')+1])
                    Path(command[command.index('-o')+1]).write_text(
                        f'200;;cycles;1000;{coverage}\n400;;instructions;1000;{coverage}\n')
                return subprocess.CompletedProcess(command, 0, '', '')
            with tempfile.TemporaryDirectory() as temp, patch.object(perf.shutil, 'which', return_value='/usr/bin/perf'), patch.object(perf.subprocess, 'run', side_effect=fake):
                result=perf.run(['true'],temp)
                self.assertNotIn('reason', result['events']['cycles'])
                self.assertEqual(result['ipc'], 2 if coverage == '100.00' else None)
                self.assertEqual(result['scaled_ipc'], None if coverage == '100.00' else 2)
                self.assertEqual(result['events']['cycles']['multiplexed'], None if not coverage else coverage != '100.00')
unittest.main()
