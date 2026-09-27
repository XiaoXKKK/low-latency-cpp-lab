"""Histogram semantics, independent of matplotlib and rendered appearance."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('plot_order_book', Path(__file__).resolve().parents[1] / 'tools/plot_order_book.py')
plot = importlib.util.module_from_spec(spec)
spec.loader.exec_module(plot)


def run(samples, repeat=0):
    return {'row': {'raw_samples': samples, 'sample_count': len(samples), 'sample_kind': 'single_event',
                    'unit': 'ns/event', 'operations_per_sample': 1}, 'repeat': repeat, 'file': f'run-{repeat}.json'}


class HistogramContract(unittest.TestCase):
    def test_shared_edges_and_rightmost_boundary(self):
        data = plot.histogram_data([('a', [run([0, 4, 5, 10, 11, 100])]),
                                    ('b', [run([1, 1, 6, 6, 20, 200])])], bin_width=5, x_max=10)
        self.assertEqual(data['edges_ns'], [0, 5, 10])
        self.assertEqual(data['series'][0]['counts'], [2, 2])
        for row in data['series']:
            self.assertEqual(sum(row['counts']) + row['overflow_count'], row['sample_count'])
        self.assertEqual(data['series'][0]['overflow_count'], 2)
        self.assertEqual(data['series'][0]['median_ns'], 7.5)  # Not median of visible samples.

    def test_probability_uses_all_samples_including_overflow(self):
        data = plot.histogram_data([('a', [run([1, 3, 100, 101])]), ('b', [run([1, 3])])],
                                   bin_width=5, x_max=5, stat='probability')
        self.assertEqual(data['series'][0]['heights'], [50])
        self.assertEqual(data['series'][1]['heights'], [100])
        self.assertEqual(data['series'][0]['median_ns'], 51.5)
        with self.assertRaisesRegex(ValueError, 'equal sample counts'):
            plot.histogram_data([('a', [run([1, 3, 100, 101])]), ('b', [run([1, 3])])])

    def test_pooling_is_explicit_and_median_is_from_pooled_events(self):
        data = plot.histogram_data([('a', [run([0, 0, 0]), run([100], 1)])], bin_width=10)
        row = data['series'][0]
        self.assertEqual(row['sample_count'], 4)
        self.assertEqual(row['median_ns'], 0)
        self.assertEqual(row['round_medians_ns'], [0, 100])
        self.assertEqual(row['round_ids'], [0, 1])

    def test_viewport_is_shared_and_rounded_up(self):
        data = plot.histogram_data([('a', [run([0, 2])]), ('b', [run([10, 12])])],
                                   bin_width=5, view_percentile=50)
        self.assertEqual(data['x_max_ns'], 15)
        self.assertEqual(plot.histogram_data([('a', [run([0, 0])])])['edges_ns'], [0, 5])

    def test_reject_invalid_or_replay_samples(self):
        for samples in [[], [-1], [float('nan')], [float('inf')]]:
            with self.assertRaises(ValueError):
                plot.histogram_data([('a', [run(samples)])])
        replay = run([10]); replay['row']['sample_kind'] = 'replay_mean'
        with self.assertRaisesRegex(ValueError, 'single-event'):
            plot.histogram_data([('replay', [replay])])
        for args in [{'bin_width': 0}, {'x_max': -1}, {'x_max': float('nan')},
                     {'view_percentile': 0}, {'bin_width': .001, 'x_max': 1000}]:
            with self.assertRaises(ValueError):
                plot.histogram_data([('a', [run([1])])], **args)
        with self.assertRaisesRegex(ValueError, 'duplicate rounds'):
            plot.histogram_data([('a', [run([1]), run([2])])])


if __name__ == '__main__':
    unittest.main()
