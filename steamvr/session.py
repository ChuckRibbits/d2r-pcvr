#!/usr/bin/env python3
"""Finish this Steam launch when D2R exits but Proton helpers linger."""
import os
from pathlib import Path
import signal
import subprocess
import sys
import time


def processes():
    found = {}
    for entry in Path('/proc').iterdir():
        if not entry.name.isdigit():
            continue
        try:
            fields = (entry / 'stat').read_text().rsplit(')', 1)[1].split()
            argv = (entry / 'cmdline').read_bytes().split(b'\0')
            name = argv[0].decode(errors='replace').strip().replace('\\', '/').rsplit('/', 1)[-1].lower()
            found[int(entry.name)] = (int(fields[1]), fields[19], fields[0], name)
        except (OSError, ValueError, IndexError):
            continue
    return found


def members(snapshot, root, known):
    # Keep known orphans, verifying start times so a reused PID cannot match.
    owned = {root} | {pid for pid, stamp in known.items()
                      if pid in snapshot and snapshot[pid][1] == stamp}
    while True:
        more = {pid for pid, data in snapshot.items() if data[0] in owned}
        if more <= owned:
            break
        owned |= more
    return {pid: snapshot[pid] for pid in owned if pid in snapshot}


def terminate_helpers(process, owned):
    snapshot = processes()
    owned = members(snapshot, process.pid, {pid: data[1] for pid, data in owned.items()})
    # Called only after all observed D2R executables have exited. Helpers are
    # confined to this launch's descendants, never the main Steam/SteamVR apps.
    for pid, data in owned.items():
        current = snapshot.get(pid)
        if current is None or current[1] != data[1] or current[2] == 'Z':
            continue
        if current[3] in ('d2r.exe', 'd2rloader.exe'):
            return False
    for pid, data in owned.items():
        current = snapshot.get(pid)
        if current is None or current[1] != data[1]:
            continue
        try:
            os.kill(pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
    try:
        process.wait(timeout=3)
    except subprocess.TimeoutExpired:
        # Re-check identities before finishing stubborn helpers.
        snapshot = processes()
        for pid, data in owned.items():
            if pid in snapshot and snapshot[pid][1] == data[1]:
                try:
                    os.kill(pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
        process.wait(timeout=3)
    return True


def run(command, poll=.5, grace=3):
    process = subprocess.Popen(command)
    known = {}
    saw_game = False
    exited_at = None
    while process.poll() is None:
        owned = members(processes(), process.pid, known)
        known.update({pid: data[1] for pid, data in owned.items()})
        games = [pid for pid, data in owned.items()
                 if data[2] != 'Z' and data[3] in ('d2r.exe', 'd2rloader.exe')]
        if games:
            saw_game = True
            exited_at = None
        elif saw_game:
            exited_at = exited_at or time.monotonic()
            if time.monotonic() - exited_at >= grace and terminate_helpers(process, owned):
                print('D2R exited; closed this launch\'s remaining Proton helpers.', flush=True)
                return 0
        time.sleep(poll)
    return process.returncode


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('Expected the Steam launch command')
    raise SystemExit(run(sys.argv[1:]))
