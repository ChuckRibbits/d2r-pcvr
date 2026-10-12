#!/usr/bin/env python3
"""Native Windows stock/VR launches and explicit Battle.net offline previews."""
import ntpath
import os
from pathlib import Path
import subprocess
import sys

RUNTIME_OPTION = '--frame-openxr-runtime='
VDXR_OPTION = '--frame-vdxr'
BATTLENET_OPTION = '--battlenet-game='
EXPERIMENTAL_OPTION = '--experimental-battlenet'


def battlenet_game(arguments):
    games = [arg[len(BATTLENET_OPTION):] for arg in arguments if arg.startswith(BATTLENET_OPTION)]
    if len(games) > 1:
        raise SystemExit('Expected at most one --battlenet-game=PATH')
    return Path(games[0]).resolve() if games else None


def prepare(arguments):
    vr = '--frame-vr' in arguments
    game = battlenet_game(arguments)
    if game is not None:
        from game_install import validate_game
        if not vr:
            raise SystemExit('Battle.net preview shortcuts require --frame-vr')
        if any(arg not in ('--frame-vr', VDXR_OPTION, EXPERIMENTAL_OPTION)
               and not arg.startswith((BATTLENET_OPTION, RUNTIME_OPTION)) for arg in arguments):
            raise SystemExit('Unexpected argument in Battle.net preview shortcut')
        try:
            validate_game(game, 'battlenet', EXPERIMENTAL_OPTION in arguments)
        except ValueError as error:
            raise SystemExit(str(error)) from error
        loader = game / 'D2RLoader.exe'
        if not loader.is_file():
            raise SystemExit('D2RLoader is not installed beside the Battle.net D2R.exe')
        # Wrapper-only flags are never passed to Blizzard's executable or loader.
        return [str(loader)], str(game)
    command = [arg for arg in arguments
               if arg not in ('--frame-vr', VDXR_OPTION) and not arg.startswith(RUNTIME_OPTION)]
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


def vdxr_manifest():
    return Path(os.environ.get('ProgramFiles', 'C:/Program Files')) / 'Virtual Desktop Streamer/OpenXR/virtualdesktop-openxr.json'


def launch_environment(arguments):
    """Allow a per-game runtime choice without changing other PCVR apps."""
    if '--frame-vr' not in arguments:
        return None
    runtimes = [arg[len(RUNTIME_OPTION):] for arg in arguments
                if arg.startswith(RUNTIME_OPTION)]
    if len(runtimes) > 1:
        raise SystemExit('Expected at most one --frame-openxr-runtime=PATH manifest')
    # The explicit launch flag overrides the normal SteamVR launch-option
    # manifest. Stock Play ignores it, and a missing VDXR install fails closed.
    environment = os.environ.copy()
    if battlenet_game(arguments) is not None:
        # Non-Steam shortcuts inherit Steam IDs too. They must not select Steam
        # ownership/authentication for a legitimately owned Battle.net copy.
        for key in list(environment):
            if key.lower() in ('steamappid', 'steamgameid', 'steamoverlaygameid'):
                del environment[key]
    if VDXR_OPTION in arguments:
        runtime = vdxr_manifest()
    elif runtimes:
        runtime = Path(runtimes[0])
    else:
        return environment if battlenet_game(arguments) is not None else None
    if not runtime.is_file():
        raise SystemExit(f'OpenXR runtime manifest was not found: {runtime}')
    environment['XR_RUNTIME_JSON'] = str(runtime.resolve())
    return environment


if __name__ == '__main__':
    command, cwd = prepare(sys.argv[1:])
    # Inherit Steam's environment and preserve its arguments. Never launch a
    # separate prefix/account or terminate a running game to finish a session.
    raise SystemExit(subprocess.run(command, cwd=cwd,
                                   env=launch_environment(sys.argv[1:])).returncode)
