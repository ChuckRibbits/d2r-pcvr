#!/usr/bin/env python3
"""Steam launch wrapper for native Windows stock and explicit VR launches."""
import ntpath
import os
from pathlib import Path
import subprocess
import sys

RUNTIME_OPTION = '--frame-openxr-runtime='


def prepare(arguments):
    vr = '--frame-vr' in arguments
    command = [arg for arg in arguments
               if arg != '--frame-vr' and not arg.startswith(RUNTIME_OPTION)]
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


def launch_environment(arguments):
    """Allow a per-game runtime choice without changing other PCVR apps."""
    runtimes = [arg[len(RUNTIME_OPTION):] for arg in arguments
                if arg.startswith(RUNTIME_OPTION)]
    if '--frame-vr' not in arguments or not runtimes:
        return None
    if len(runtimes) != 1 or not Path(runtimes[0]).is_file():
        raise SystemExit('Expected one existing --frame-openxr-runtime=PATH manifest')
    environment = os.environ.copy()
    environment['XR_RUNTIME_JSON'] = str(Path(runtimes[0]).resolve())
    return environment


if __name__ == '__main__':
    command, cwd = prepare(sys.argv[1:])
    # Inherit Steam's environment and preserve its arguments. Never launch a
    # separate prefix/account or terminate a running game to finish a session.
    raise SystemExit(subprocess.run(command, cwd=cwd,
                                   env=launch_environment(sys.argv[1:])).returncode)
