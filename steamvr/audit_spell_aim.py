#!/usr/bin/env python3
"""Read-only audit of spell aim hooks and optional local compiled skill layout.

Game snapshots and source/compiled tables stay local; none are distributed.
"""
import argparse
import csv
from pathlib import Path
import re
import struct


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('snapshot', type=Path)
    ap.add_argument('--skills-bin', type=Path)
    ap.add_argument('--skills-txt', type=Path)
    ap.add_argument('--missiles-txt', type=Path)
    args = ap.parse_args()
    header = (Path(__file__).resolve().parents[1] / 'vr/spell_aim_signatures.h').read_text()
    data = args.snapshot.read_bytes()
    count = 0
    for name, address in re.findall(r'uint64_t (\w+)Rva=(0x[0-9a-f]+);', header):
        encoded = re.search(r'uint8_t ' + name + r'Sig\[\]=\{([^}]+)\}', header)[1]
        expected = bytes(int(v, 16) for v in encoded.split(','))
        offset = int(address, 16) - 0x1000
        if offset < 0 or data[offset:offset + len(expected)] != expected:
            raise SystemExit(f'Signature mismatch: {name} at {address}')
        count += 1
    if count != 7:
        raise SystemExit('Incomplete spell aim guard set')
    print(f'Spell aiming: {count} native entry/call-site guards match')
    paths = (args.skills_bin, args.skills_txt, args.missiles_txt)
    if not any(paths):
        return
    if not all(paths):
        raise SystemExit('Supply all three local table files for the metadata audit')
    skills = list(csv.DictReader(args.skills_txt.read_text().splitlines(), delimiter='\t'))
    missiles = {r['Missile']: int(r['*ID']) for r in csv.DictReader(
        args.missiles_txt.read_text().splitlines(), delimiter='\t') if r.get('*ID', '').isdigit()}
    raw = args.skills_bin.read_bytes()
    entries = struct.unpack_from('<I', raw)[0]
    if entries < 300 or len(raw) != 4 + entries * 748:
        raise SystemExit('Unsupported compiled Skills layout')
    checked = 0
    for row in skills:
        if not row.get('*Id', '').isdigit():
            continue
        i = int(row['*Id'])
        if i >= entries:
            raise SystemExit('Skill index exceeds compiled table')
        base = 4 + i * 748
        for offset, expected in ((0, i), (0x4e, int(row['srvdofunc'] or 0)),
                                 (0x66, missiles.get(row['srvmissile'], -1))):
            if struct.unpack_from('<h', raw, base + offset)[0] != expected:
                raise SystemExit(f'Skill {i}: field mismatch at {offset:#x}')
        flags = struct.unpack_from('<I', raw, base + 0x24)[0]
        for bit, name in ((4, 'passive'), (20, 'TargetableOnly'), (24, 'TargetCorpse')):
            if ((flags >> bit) & 1) != int(row[name] or 0):
                raise SystemExit(f'Skill {i}: flag mismatch for {name}')
        checked += 1
    if checked != entries:
        raise SystemExit('Incomplete Skills metadata audit')
    print(f'Spell aiming: behavior/targeting fields match all {checked} native skills')


if __name__ == '__main__':
    main()
