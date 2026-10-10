#!/usr/bin/env python3
"""Read saved SteamVR Link timing dumps without connecting to a headset.

These are fault-triggered, overlapping histories, not a continuous benchmark.
Receive timestamps are the driver's clock-aligned reports: delays can include
transport/retransmission/client handling, not just radio propagation. The CSV
does not establish actual image resolution, bitrate or a packet-loss rate.
"""
import argparse
import csv
from datetime import datetime
import json
from pathlib import Path
import re
import statistics

WRAP = 1 << 32  # Driver microsecond timestamps wrap within long VR sessions.
MAX_INTERVAL_US = 60_000_000
REALTIME = 'WGP Release (realtime)'
STAGES = {
    'gate_to_app_submit': ('WGP Release', 'App Submit'),
    'encode': ('Encode Start', 'Encode End'),
    'send': ('Send Start', 'Send End'),
    'send_to_reported_receive': ('Send Start', 'Receive'),
    'receive_to_decode': ('Receive', 'Decode Start'),
    'decode': ('Decode Start', 'Decode End'),
}
REQUIRED = {REALTIME, 'Present ID', 'TX ID'} | {
    column for pair in STAGES.values() for column in pair}


def interval_us(start, end):
    if start <= 0 or end <= 0:
        return None  # An absent timestamp must never become zero work.
    delta = end - start
    if delta < 0 and WRAP - MAX_INTERVAL_US < start < WRAP and end < MAX_INTERVAL_US:
        delta += WRAP
    return delta if 0 <= delta <= MAX_INTERVAL_US else None


def merge(paths, since=None, until=None):
    rows, raw, invalid = {}, 0, 0
    for path in sorted(map(Path, paths)):
        with path.open(newline='', encoding='utf-8-sig') as stream:
            reader = csv.DictReader(stream)
            missing = REQUIRED - set(reader.fieldnames or [])
            if missing:
                raise ValueError(f'{path.name}: missing columns: {", ".join(sorted(missing))}')
            for row in reader:
                raw += 1
                try:
                    values = {k: int(v) for k, v in row.items()}
                except (TypeError, ValueError):
                    invalid += 1
                    continue
                wall = values[REALTIME]
                if wall <= 0:
                    invalid += 1
                    continue
                if (since is not None and wall < since) or (until is not None and wall > until):
                    continue
                # Present IDs repeat when the driver has no new game image.
                # TX can be absent in one dump and filled in a later history.
                key = (wall, values['Present ID'])
                def completeness(r):
                    return (sum(r.get(k, 0) > 0 for k in REQUIRED - {'TX ID'}), r['TX ID'] >= 0)
                if key not in rows or completeness(values) >= completeness(rows[key]):
                    rows[key] = values
    return sorted(rows.values(), key=lambda r: r[REALTIME]), raw, invalid


def stats(values):
    if not values:
        return {'samples': 0, 'median_ms': None, 'p95_ms': None, 'max_ms': None}
    values = sorted(values)
    return {'samples': len(values), 'median_ms': statistics.median(values),
            'p95_ms': values[int((len(values) - 1) * .95)], 'max_ms': values[-1]}


def iso(us):
    return datetime.fromtimestamp(us / 1e6).astimezone().isoformat(timespec='milliseconds')


def summarize(rows):
    stages = {}
    for name, (start, end) in STAGES.items():
        values = [interval_us(r[start], r[end]) for r in rows
                  if name == 'gate_to_app_submit' or r['TX ID'] >= 0]
        stages[name] = stats([v / 1000 for v in values if v is not None])
    delays = [(interval_us(r['Send Start'], r['Receive']), r) for r in rows if r['TX ID'] >= 0]
    delays = [(v, r) for v, r in delays if v is not None]
    worst = []
    for delay, row in sorted(delays, key=lambda item: item[0], reverse=True)[:10]:
        event = {'at': iso(row[REALTIME]), 'present_id': row['Present ID'], 'tx_id': row['TX ID']}
        for name, (start, end) in STAGES.items():
            value = interval_us(row[start], row[end])
            event[name + '_ms'] = None if value is None else value / 1000
        worst.append(event)
    return {'unique_rows': len(rows), 'first': iso(rows[0][REALTIME]) if rows else None,
            'last': iso(rows[-1][REALTIME]) if rows else None,
            'transmitted_rows': sum(r['TX ID'] >= 0 for r in rows), 'stages': stages,
            'receive_delay_over_100ms': sum(v > 100_000 for v, _ in delays),
            'worst_receive_events': worst,
            'limits': 'Fault-triggered histories; not an unbiased session FPS/loss measurement. '
                      'Receive is driver-aligned, not pure network latency. No bitrate or resolution data.'}


def reset_events(path, since, until):
    times = []
    pattern = re.compile(r'^\w{3} (\w{3} +\d+ \d{4} \d\d:\d\d:\d\d\.\d+) ')
    for line in Path(path).read_text(errors='replace').splitlines():
        if 'Reset video stream because' not in line:
            continue
        match = pattern.match(line)
        if not match:
            continue
        wall = round(datetime.strptime(match[1], '%b %d %Y %H:%M:%S.%f').timestamp() * 1e6)
        if (since is None or wall >= since) and (until is None or wall <= until):
            times.append(wall)
    return {'count': len(times), 'at': [iso(t) for t in times]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv', nargs='+', type=Path)
    parser.add_argument('--driver-log', type=Path)
    parser.add_argument('--since', help='ISO local time or time with UTC offset; filters row release time')
    parser.add_argument('--until', help='ISO local time or time with UTC offset')
    parser.add_argument('--json', type=Path, help='Write the full report, including worst-event timestamps')
    args = parser.parse_args()
    since = round(datetime.fromisoformat(args.since).timestamp()*1e6) if args.since else None
    until = round(datetime.fromisoformat(args.until).timestamp()*1e6) if args.until else None
    if since is not None and until is not None and since > until:
        parser.error('--since must precede --until')
    try:
        rows, raw, invalid = merge(args.csv, since, until)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    report = summarize(rows)
    report.update(raw_rows=raw, invalid_rows=invalid, files=[p.name for p in args.csv])
    if args.driver_log:
        report['stream_resets'] = reset_events(args.driver_log,
            since if since is not None else rows[0][REALTIME] if rows else None,
            until if until is not None else rows[-1][REALTIME] if rows else None)
    if args.json:
        args.json.write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(rows)} unique rows from {len(args.csv)} histories ({raw} input, {invalid} invalid).')
    print(f'{report["first"]} to {report["last"]}')
    print(f'{"Stage":28s} {"Samples":>8s} {"Median ms":>10s} {"p95 ms":>10s} {"Max ms":>10s}')
    for name, data in report['stages'].items():
        numbers = ['n/a' if data[k] is None else f'{data[k]:.3f}' for k in ('median_ms','p95_ms','max_ms')]
        print(f'{name:28s} {data["samples"]:8d} ' + ' '.join(f'{v:>10s}' for v in numbers))
    if args.driver_log:
        print(f'Stream reset requests in selected interval: {report["stream_resets"]["count"]}')
    print(report['limits'])


if __name__ == '__main__':
    main()
