import unittest
from profile_report import COLUMNS, records, hook_records, late_pairs, timing_text


class ReportTest(unittest.TestCase):
    def test_missing_samples_are_not_reported_as_free_work(self):
        self.assertIn('n/a', timing_text(0, 0))
        self.assertIn('0.000', timing_text(1, 0))

    def test_older_logs_still_parse(self):
        row = [1, 1] + [0] * (len(COLUMNS) - 2)
        frames, scopes = records('prefix Frame profile,' + ','.join(map(str, row)))
        self.assertEqual(len(frames), 1)
        self.assertEqual(scopes, {})

    def test_workers_and_sequences_remain_separate(self):
        text = ('Frame hook,1,120,42,memory-write,1200,18,0.100,14400\n'
                'Frame hook,1,360,42,memory-write,3600,56,0.080,43200\n'
                'Frame hook,1,360,43,memory-write,7200,112,0.130,86400\n'
                'Frame hook,2,120,42,memory-write,2400,36,0.110,28800\n')
        hooks = hook_records(text)
        self.assertEqual(len(hooks), 3)
        self.assertEqual(hooks[(1, 42, 'memory-write')], (360, 3600, 56, .080, 43200))

    def test_budget_counts_include_idle_wait(self):
        rows = [dict(gap=1, begin=2, left=7, right=7, end=3),
                dict(gap=1, begin=4, left=7, right=7, end=4)]
        self.assertEqual(late_pairs(rows, 22.222), 1)
        self.assertEqual(late_pairs(rows, 18.519), 2)


if __name__ == '__main__':
    unittest.main()
