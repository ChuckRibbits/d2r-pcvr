"""Read-only baseline-address check against an owner's running Windows D2R.

This diagnostic never injects, hooks, writes memory or reads account/save data.
Only readable signature sites are inspected; protected pages are left alone.
Results are preliminary: loader patches and deferred code need a loader run,
and matching bytes do not establish gameplay compatibility.
"""
import argparse
from collections import Counter
import ctypes as C
import json
import os
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def guards():
    for header in sorted((ROOT / 'vr').glob('*_signatures.h')):
        text = header.read_text()
        arrays = dict(re.findall(r'uint8_t\s+(\w+)Sig\[\]\s*=\s*\{([^}]+)\}', text))
        for name, rva in re.findall(r'uint64_t\s+(\w+)Rva\s*=\s*(0x[0-9a-fA-F]+)', text):
            if name in arrays:
                data = bytes(int(value.strip(), 0) for value in arrays[name].split(',') if value.strip())
                yield header.name, name, int(rva, 16), list(data)
    text = (ROOT / 'cleanroom/sigscan/game_sigs.h').read_text()
    for name, pattern, site in re.findall(r'\{"(\w+)",\s*0x[0-9a-fA-F]+,\s*Kind::\w+,\s*"([0-9a-fA-F? ]+)",\s*(0x[0-9a-fA-F]+)', text):
        yield 'game_sigs.h', name, int(site, 16), [None if b == '??' else int(b, 16) for b in pattern.split()]


def check(data, pattern):
    if data is None or len(data) != len(pattern):
        return 'unreadable'
    return 'matches' if all(want is None or got == want for got, want in zip(data, pattern)) else 'different_at_baseline_address'


class Process:
    def __init__(self, pid, game):
        if os.name != 'nt':
            raise ValueError('Live audit requires native Windows')
        self.kernel = k = C.WinDLL('kernel32', use_last_error=True)
        k.OpenProcess.argtypes = [C.c_uint32, C.c_bool, C.c_uint32]
        k.OpenProcess.restype = C.c_void_p
        k.CloseHandle.argtypes = [C.c_void_p]
        k.QueryFullProcessImageNameW.argtypes = [C.c_void_p, C.c_uint32, C.c_wchar_p, C.POINTER(C.c_uint32)]
        k.ReadProcessMemory.argtypes = [C.c_void_p, C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
        k.ReadProcessMemory.restype = C.c_int
        self.handle = k.OpenProcess(0x410, False, pid)  # QUERY_INFORMATION | VM_READ only
        if not self.handle:
            raise C.WinError(C.get_last_error())
        try:
            path, size = C.create_unicode_buffer(32768), C.c_uint32(32768)
            if not k.QueryFullProcessImageNameW(self.handle, 0, path, C.byref(size)):
                raise C.WinError(C.get_last_error())
            self.executable = Path(path.value).resolve()
            if (self.executable.parent != Path(game).resolve()
                    or self.executable.name.lower() not in ('d2r.exe', 'd2rloader.exe')):
                raise ValueError('PID does not belong to D2R in the specified game folder')
            psapi = C.WinDLL('psapi', use_last_error=True)
            psapi.EnumProcessModulesEx.argtypes = [C.c_void_p, C.POINTER(C.c_void_p), C.c_uint32, C.POINTER(C.c_uint32), C.c_uint32]
            modules, needed = (C.c_void_p * 1024)(), C.c_uint32()
            if not psapi.EnumProcessModulesEx(self.handle, modules, C.sizeof(modules), C.byref(needed), 3) or not needed.value:
                raise C.WinError(C.get_last_error())
            self.base = modules[0]
        except BaseException:
            self.close()
            raise

    def read(self, rva, size):
        data, got = C.create_string_buffer(size), C.c_size_t()
        if self.kernel.ReadProcessMemory(self.handle, self.base + rva, data, size, C.byref(got)) and got.value == size:
            return data.raw
        return None

    def close(self):
        if self.handle:
            self.kernel.CloseHandle(self.handle)
            self.handle = None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--game', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True, help='Local JSON summary; no game code is saved')
    args = parser.parse_args()
    process = Process(args.pid, args.game)
    try:
        results = [{'group': group, 'name': name, 'baseline_rva': hex(rva),
                    'status': check(process.read(rva, len(pattern)), pattern)}
                   for group, name, rva, pattern in guards()]
        result = {'executable': str(process.executable), 'pid': args.pid,
                  'read_only': True, 'baseline': 'Steam 3.3.93787 under D2RLoader 1.3.1',
                  'counts': dict(Counter(item['status'] for item in results)), 'guards': results,
                  'limitation': 'Preliminary signature-site check; loader patches, relocation, structure layouts and offline gameplay still require validation.'}
    finally:
        process.close()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result['counts']))


if __name__ == '__main__':
    main()
