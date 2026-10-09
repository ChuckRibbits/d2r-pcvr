#!/usr/bin/env python3
"""Steam launch wrapper for the untested native Windows preview."""
import ntpath
from pathlib import Path
import subprocess
import sys


def prepare(arguments):
    vr = '--frame-vr' in arguments
    command = [arg for arg in arguments if arg != '--frame-vr']
    cwd = None
    if not command:
        raise SystemExit('Expected the Steam launch command containing D2R.exe')
    if vr:
        found = False
        for index, arg in enumerate(command):
            if ntpath.basename(arg).lower() == 'd2r.exe':
                folder = ntpath.dirname(arg)
                loader = ntpath.join(folder, 'D2RLoader.exe')
                if not Path(loader).is_file():
                    raise SystemExit('D2RLoader is not installed beside D2R.exe')
                command[index] = loader
                cwd = folder
                found = True
        if not found:
            raise SystemExit('Expected the Steam launch command containing D2R.exe')
    return command, cwd


if __name__ == '__main__':
    command, cwd = prepare(sys.argv[1:])
    # Inherit Steam's environment and preserve its arguments. Never launch a
    # separate prefix/account or terminate a running game to finish a session.
    raise SystemExit(subprocess.run(command, cwd=cwd).returncode)
