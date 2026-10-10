import csv
from pathlib import Path
import tempfile
import unittest

from stream_report import REALTIME, REQUIRED, WRAP, interval_us, merge, summarize, reset_events


class StreamReportTest(unittest.TestCase):
    def test_missing_reversed_and_wrapped_timestamps(self):
        self.assertIsNone(interval_us(0, 15000))
        self.assertIsNone(interval_us(100, 0))
        self.assertIsNone(interval_us(12000, 11000))
        self.assertIsNone(interval_us(1, 90_000_001))
        self.assertEqual(interval_us(WRAP - 3000, 2000), 5000)
        self.assertEqual(interval_us(3000, 3000), 0)

    def test_overlap_prefers_completed_rows_but_retains_repeated_presents(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = dict.fromkeys(REQUIRED, 0)
            a.update({REALTIME: 1_700_000_000_000_000, 'Present ID': 99, 'TX ID': -1})
            b = dict(a, **{'TX ID': 8, 'Encode Start': 100, 'Encode End': 8000})
            c = dict(b, **{REALTIME: a[REALTIME]+22_222})
            paths = [Path(tmp)/'a.csv', Path(tmp)/'b.csv']
            for path, rows in zip(paths, ([a], [b, c])):
                with path.open('w') as stream:
                    writer = csv.DictWriter(stream, fieldnames=sorted(REQUIRED))
                    writer.writeheader(); writer.writerows(rows)
            rows, raw, invalid = merge(paths)
            self.assertEqual((len(rows), raw, invalid), (2, 3, 0))
            self.assertEqual(rows[0]['TX ID'], 8)
            self.assertEqual(len(merge(paths, since=c[REALTIME], until=c[REALTIME])[0]), 1)

    def test_receiver_stall_does_not_become_encode_or_game_cost(self):
        row = dict.fromkeys(REQUIRED, 0)
        row.update({REALTIME: 1_700_000_000_000_000, 'Present ID': 1, 'TX ID': 1,
                    'WGP Release': 1000, 'App Submit': 18000,
                    'Encode Start': 18010, 'Encode End': 26010,
                    'Send Start': 26011, 'Send End': 26511, 'Receive': 626011})
        report = summarize([row])
        self.assertEqual(report['stages']['gate_to_app_submit']['max_ms'], 17)
        self.assertEqual(report['stages']['encode']['max_ms'], 8)
        self.assertEqual(report['stages']['send_to_reported_receive']['max_ms'], 600)
        self.assertEqual(report['stages']['decode']['samples'], 0)
        self.assertIsNone(report['stages']['decode']['max_ms'])
        self.assertEqual(report['receive_delay_over_100ms'], 1)

    def test_faulty_rows_and_missing_schema(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/'a.csv'
            path.write_text('wrong,columns\n1,2\n')
            with self.assertRaises(ValueError):
                merge([path])
            with path.open('w') as stream:
                writer = csv.DictWriter(stream, fieldnames=sorted(REQUIRED))
                writer.writeheader(); writer.writerow(dict.fromkeys(REQUIRED, 'bad'))
                writer.writerow(dict.fromkeys(REQUIRED, 0))
            self.assertEqual(merge([path]), ([], 2, 2))
            self.assertEqual(summarize([])['unique_rows'], 0)

    def test_reset_requests_counted_once_and_scoped(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/'driver.txt'
            path.write_text('Sat Oct 10 2026 02:20:00.000000 [Info] - Reset video stream because x\n'
                            'Sat Oct 10 2026 02:20:00.001000 [Info] - SVLFEC::ReportStreamReset()\n'
                            'Sat Oct 10 2026 02:21:00.000000 [Info] - Reset video stream because x\n')
            from datetime import datetime
            start = round(datetime(2026,10,10,2,20,30).timestamp()*1e6)
            self.assertEqual(reset_events(path,None,None)['count'], 2)
            self.assertEqual(reset_events(path,start,None)['count'], 1)


if __name__ == '__main__':
    unittest.main()
