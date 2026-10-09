#!/usr/bin/env python3
"""Compare our NGX calls with Clang's MSVC ABI, without a GPU or Windows SDK.

Run with --clang /path/to/clang++. All 17 SDK virtual calls must compile to the
same instructions for our adapter under both Windows targets as the original
overloaded SDK interface under MSVC. The old interface under MinGW must differ,
so the test also demonstrates the regression it protects against.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clang", default="clang++")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    # Declaration order of NVIDIA's public nvsdk_ngx_params.h (not slot order).
    types = ["unsigned long long", "float", "double", "unsigned int", "int",
             "ID3D11Resource*", "ID3D12Resource*", "void*"]
    reference = "struct ID3D11Resource; struct ID3D12Resource;\nstruct Params {\n"
    reference += "\n".join(f"virtual void Set(const char*, {t}) = 0;" for t in types)
    reference += "\n" + "\n".join(
        f"virtual int Get(const char*, {t}*) const = 0;" for t in types)
    reference += "\nvirtual void Reset() = 0;\n};\n"
    adapter = '#include "vr/ngx_parameter.h"\nusing Params = dlssmv::NgxParameter;\n'
    calls = ""
    names = []
    for i, t in enumerate(types):
        names.extend([f"set_{i}", f"get_{i}"])
        calls += (f'extern "C" void set_{i}(Params* p, const char* n, {t} v) '
                  '{ p->Set(n, v); }\n')
        calls += (f'extern "C" int get_{i}(const Params* p, const char* n, {t}* v) '
                  '{ return p->Get(n, v); }\n')
    names.append("reset")
    calls += 'extern "C" void reset(Params* p) { p->Reset(); }\n'

    def compile_calls(directory, source, abi):
        result = subprocess.run(
            [args.clang, f"--target=x86_64-windows-{abi}", "-std=c++20", "-O2",
             "-S", "-x", "c++", "-", "-o", "-", "-I", str(root)],
            input=source + calls, capture_output=True, text=True, check=True,
            cwd=directory)
        bodies = {}
        for name in names:
            body = result.stdout.split(name + ":", 1)[1].split("# -- End function", 1)[0]
            # Compare actual instructions, including argument registers and
            # indirect-call offsets; discard comments and assembler directives.
            instructions = []
            for line in body.splitlines():
                line = line.split("#", 1)[0].strip()
                if line and not line.startswith("."):
                    instructions.append(re.sub(r"\s+", " ", line))
            bodies[name] = instructions
        return bodies

    with tempfile.TemporaryDirectory(prefix="ngx-abi-") as directory:
        expected = compile_calls(directory, reference, "msvc")
        broken = compile_calls(directory, reference, "gnu")
        assert sum(broken[n] != expected[n] for n in names) == 16, \
            "The reference no longer reproduces the MinGW overload mismatch"
        for abi in ("gnu", "msvc"):
            actual = compile_calls(directory, adapter, abi)
            for name in names:
                assert actual[name] == expected[name], \
                    f"{abi} {name}: {actual[name]} != MSVC {expected[name]}"
            print(f"{abi}: all 17 NGX calls match the MSVC SDK ABI")


if __name__ == "__main__":
    main()
