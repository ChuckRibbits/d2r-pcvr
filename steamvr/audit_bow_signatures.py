#!/usr/bin/env python3
"""Check bow ABI guards against an owner's decrypted game-code snapshot.

The snapshot is a local diagnostic artifact, never a distributed game binary.
Example: python3 steamvr/audit_bow_signatures.py build-frame/game-code.bin
"""
import argparse
from pathlib import Path
import re


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot", type=Path)
    parser.add_argument("--start-rva", type=lambda value: int(value, 0), default=0x1000)
    args = parser.parse_args()
    header = (Path(__file__).resolve().parents[1] / "vr/bow_signatures.h").read_text()
    data = args.snapshot.read_bytes()
    count = 0
    for name, rva in re.findall(r"uint64_t (\w+)Rva=(0x[0-9a-f]+);", header):
        values = re.search(r"uint8_t " + name + r"Sig\[\]=\{([^}]+)\}", header)[1]
        expected = bytes(int(value, 16) for value in values.split(","))
        offset = int(rva, 16) - args.start_rva
        if offset < 0 or data[offset:offset + len(expected)] != expected:
            raise SystemExit(f"Mismatch: {name} at RVA {rva}")
        count += 1
    if count < 20:
        raise SystemExit("Incomplete bow signature/layout guard set")
    print(f"Physical bow: {count} native entry/call-site/layout signatures match")


if __name__ == "__main__":
    main()
