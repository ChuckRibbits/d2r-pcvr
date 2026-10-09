#!/usr/bin/env python3
"""Summarize bounded live diagnostics; all timings are milliseconds.

GPU scene spans include idle queue time while the CPU records commands.
Sampled callback estimates are inclusive and must not be summed as exclusive
CPU categories. Only the thread drawing the stereo pair is instrumented.
"""
import argparse
import csv
from pathlib import Path
import statistics

COLUMNS = ('sequence frame gap begin left right end cpu_left cpu_right wait_frame '
           'locate input acquire_left acquire_right copy_submit_left copy_submit_right '
           'gpu_fence release submit gpu_scene_left gpu_scene_right gpu_effects_left '
           'gpu_effects_right gpu_copy_left gpu_copy_right gpu_valid').split()


def records(text):
    frames, scopes = [], {}
    for line in text.splitlines():
        if 'Frame profile,' in line:
            row = next(csv.reader([line.split('Frame profile,', 1)[1]]))
            if len(row) != len(COLUMNS):
                raise ValueError(f'Profile field count {len(row)}, expected {len(COLUMNS)}')
            frames.append(dict(zip(COLUMNS, map(float, row))))
        elif 'Frame scope,' in line:
            seq, count, eye, name, calls, samples, ms = line.split('Frame scope,', 1)[1].split(',')
            scopes[(int(seq), int(eye), name)] = (int(count), int(calls), int(samples), float(ms))
    return frames, scopes


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('log', type=Path)
    ap.add_argument('--sequence', type=int)
    ap.add_argument('--csv', type=Path)
    args = ap.parse_args()
    frames, scopes = records(args.log.read_text(errors='replace'))
    if not frames:
        raise SystemExit('No profile frames recorded yet')
    seq = args.sequence if args.sequence is not None else int(frames[-1]['sequence'])
    rows = [r for r in frames if r['sequence'] == seq]
    if not rows:
        raise SystemExit(f'No frames for sequence {seq}')
    print(f'Sequence {seq}: {len(rows)} pairs; {sum(r["gpu_valid"] != 0 for r in rows)} valid GPU samples')
    print(f'{"stage":24s} {"mean":>9s} {"median":>9s} {"p95":>9s} {"max":>9s}')
    for key in ['cycle'] + COLUMNS[2:-1]:
        values = [(sum(r[k] for k in ('gap', 'begin', 'left', 'right', 'end')) if key == 'cycle' else r[key])
                  for r in rows if not key.startswith(('gpu_scene', 'gpu_effects', 'gpu_copy')) or r['gpu_valid']]
        if not values:
            continue
        ordered = sorted(values)
        print(f'{key:24s} {statistics.mean(values):9.3f} {statistics.median(values):9.3f} '
              f'{ordered[min(len(ordered)-1, int(.95*len(ordered)))]:9.3f} {max(values):9.3f}')
    print('Scopes: estimated inclusive ms/eye; calls/pair; samples (main render thread only)')
    for (scope_seq, eye, name), (count, calls, samples, ms) in sorted(scopes.items()):
        if scope_seq == seq:
            print(f'  eye {eye} {name:18s} {ms:8.3f} ms {calls/count:9.1f} calls {samples:8d} samples')
    if args.csv:
        with args.csv.open('w', newline='') as out:
            w = csv.DictWriter(out, fieldnames=COLUMNS); w.writeheader(); w.writerows(rows)


if __name__ == '__main__':
    main()
