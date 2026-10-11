#!/usr/bin/env python3
"""Player-facing Windows setup; all prompts run before any game/config writes."""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import subprocess
import sys

import install
import steam_config

ROOT = Path(__file__).resolve().parent.parent


def choose(title, options, default=0):
    print('\n' + title)
    for index, option in enumerate(options):
        print(f'  {index + 1}. {option}' + (' [default]' if index == default else ''))
    while True:
        answer = input('Choose a number (Enter for default): ').strip()
        if not answer:
            return default
        if answer.isdecimal() and 1 <= int(answer) <= len(options):
            return int(answer) - 1
        print('Enter one of the numbers above.')


def folder(title, default=None, required=None):
    while True:
        answer = input(f'{title}' + (f' [{default}]' if default else '') + ': ').strip().strip('"')
        if not answer and default is not None:
            path = default
        elif not answer:
            continue
        else:
            path = Path(os.path.expandvars(answer)).expanduser().resolve()
        if path.is_dir() and (required is None or (path / required).is_file()):
            return path
        print(f'Folder not found' + (f' or missing {required}' if required else '') + '. Please try again.')


def steam_location():
    import winreg
    for hive, key, name in [(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam', 'SteamPath'),
                            (winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\WOW6432Node\Valve\Steam', 'InstallPath')]:
        try:
            with winreg.OpenKey(hive, key) as handle:
                candidate = Path(winreg.QueryValueEx(handle, name)[0])
            if (candidate / 'steam.exe').is_file():
                return candidate.resolve()
        except OSError:
            pass
    return None


def libraries(steam):
    result = [steam]
    source = steam / 'steamapps/libraryfolders.vdf'
    if source.is_file():
        tree = steam_config.text_value(steam_config.parse_text(source.read_text(encoding='utf-8-sig')), 'libraryfolders')
        for entry in tree or []:
            if entry.key.isdecimal():
                value = steam_config.text_value(entry.value, 'path') if isinstance(entry.value, list) else entry.value
                if isinstance(value, str):
                    path = Path(value)
                    if path.is_dir() and path not in result:
                        result.append(path)
    return result


def app_folder(library_roots, app_id):
    for library in library_roots:
        source = library / f'steamapps/appmanifest_{app_id}.acf'
        if source.is_file():
            name = steam_config.text_value(steam_config.parse_text(source.read_text(encoding='utf-8-sig')), 'AppState', 'installdir')
            if isinstance(name, str):
                candidate = (library / 'steamapps/common' / name).resolve()
                common = (library / 'steamapps/common').resolve()
                if candidate.is_relative_to(common) and candidate.is_dir():
                    return candidate
    return None


def profiles(steam):
    result = []
    for profile in sorted((steam / 'userdata').glob('*')):
        source = profile / 'config/localconfig.vdf'
        if profile.name.isdecimal() and profile.name != '0' and source.is_file():
            tree = steam_config.parse_text(source.read_text(encoding='utf-8-sig'))
            name = steam_config.text_value(tree, 'UserLocalConfigStore', 'friends', 'PersonaName')
            result.append((profile, name if isinstance(name, str) else 'Steam account'))
    return result


def saved_games():
    # SHGetKnownFolderPath handles redirected Saved Games folders on Windows.
    import ctypes
    import uuid
    guid = (ctypes.c_ubyte * 16).from_buffer_copy(uuid.UUID('4c5c32ff-bb9d-43b0-b5b4-2d72e54eaaa4').bytes_le)
    value = ctypes.c_wchar_p()
    shell = ctypes.WinDLL('shell32')
    shell.SHGetKnownFolderPath.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_void_p, ctypes.POINTER(ctypes.c_wchar_p)]
    shell.SHGetKnownFolderPath.restype = ctypes.c_long
    if shell.SHGetKnownFolderPath(ctypes.byref(guid), 0, None, ctypes.byref(value)) == 0:
        try:
            return Path(value.value) / 'Diablo II Resurrected'
        finally:
            ole = ctypes.WinDLL('ole32')
            ole.CoTaskMemFree.argtypes = [ctypes.c_void_p]
            ole.CoTaskMemFree(ctypes.cast(value, ctypes.c_void_p))
    return Path.home() / 'Saved Games/Diablo II Resurrected'


def require_closed():
    rows = csv.reader(subprocess.check_output(['tasklist', '/FO', 'CSV', '/NH'], text=True).splitlines())
    if any(row and row[0].lower() in ('steam.exe', 'd2r.exe', 'd2rloader.exe') for row in rows):
        raise ValueError('Save & Exit D2R, then choose Steam > Exit on the PC. Do not just close its window.')


def wait_for_closed():
    while True:
        try:
            require_closed()
            return
        except ValueError as error:
            print('\n' + str(error))
            input('Press Enter after both are fully closed (Ctrl+C to cancel): ')


def verify_release(root):
    source = root / 'release-manifest.json'
    if not source.is_file():
        raise ValueError('Extract the complete Windows release ZIP first, then run Install-Windows.cmd from that folder.')
    manifest = json.loads(source.read_text(encoding='utf-8'))
    if manifest.get('platform') != 'native-windows':
        raise ValueError('This setup needs the native Windows release ZIP.')
    hashes = manifest['sha256']
    required = {'bin/d2rl-vrcam.dll', 'bin/openxr_loader.dll', 'steamvr/install.py',
                'steamvr/setup_windows.py', 'steamvr/steam_config.py', 'steamvr/launch_windows.py'}
    if not required.issubset(hashes):
        raise ValueError('Incomplete Windows setup package')
    for name, digest in hashes.items():
        relative = PurePosixPath(name.replace('\\', '/'))
        if relative.is_absolute() or '..' in relative.parts or ':' in name:
            raise ValueError('Invalid path in release manifest')
        path = root / relative
        if not path.resolve().is_relative_to(root.resolve()) or not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise ValueError(f'Release file is missing or changed: {name}. Extract a fresh ZIP.')
    return manifest


def launch_options(python, wrapper, runtime):
    parts = [str(python), str(wrapper)]
    if runtime is not None:
        parts.append('--frame-openxr-runtime=' + str(runtime))
    if any('"' in part or '\n' in part or '\r' in part or '%' in part for part in parts):
        raise ValueError('Move the extracted mod to a folder without quotes, percent signs or line breaks.')
    return ' '.join('"' + part + '"' for part in parts) + ' %command%'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Check extracted release files without installing')
    args = parser.parse_args()
    manifest = verify_release(ROOT)
    if args.check:
        print('Windows release files verified. No game or Steam configuration was changed.')
        return
    if os.name != 'nt':
        raise ValueError('Run Install-Windows.cmd on your Windows PC.')
    print('\nD2R PCVR - Windows setup')
    print('This mod requires your separately purchased, legitimate Steam copy of D2R 3.3.93787.')
    print('No game assets are supplied. Use a local OFFLINE character while playing this preview.')
    print('Launch stock D2R once and finish its account prompts before continuing.')
    print('Keep this extracted mod folder in a permanent location; Steam will use it to launch.')
    headset = ['Steam Frame', 'Quest 3', 'Quest 2', 'Other SteamVR headset'][choose('Which headset will you use?', ['Steam Frame', 'Quest 3', 'Quest 2', 'Other SteamVR headset'])]
    connections = ['steamvr']
    if headset in ('Quest 2', 'Quest 3'):
        if headset == 'Quest 2':
            print('Quest 2 uses the Quest controls; compatibility is not yet confirmed.')
        connection = choose('How will the Quest connect to this PC?', ['Steam Link / SteamVR', 'Virtual Desktop / VDXR', 'Both (create two shortcuts)'])
        connections = [['steamvr'], ['vdxr'], ['steamvr', 'vdxr']][connection]
    steam = folder('Steam installation folder', steam_location(), 'steam.exe')
    roots = libraries(steam)
    game = folder('D2R game folder (Steam > Properties > Installed Files > Browse)', app_folder(roots, steam_config.APP_ID), 'D2R.exe')
    if '3.3.93787'.encode('utf-16-le') not in (game / 'D2R.exe').read_bytes():
        raise ValueError('This release requires Steam D2R 3.3.93787. Your game version does not match.')
    runtime = None
    if 'steamvr' in connections:
        runtime_folder = folder('SteamVR installation folder', app_folder(roots, '250820'), 'steamxr_win64.json')
        runtime = runtime_folder / 'steamxr_win64.json'
    if 'vdxr' in connections:
        from launch_windows import vdxr_manifest
        if not vdxr_manifest().is_file():
            raise ValueError('Install Virtual Desktop Streamer in its default Windows location first: https://www.vrdesktop.net/ . Then rerun setup.')
        print('Use Virtual Desktop on Quest with its Windows Streamer. Keep SteamVR closed for VDXR.')
    accounts = profiles(steam)
    if not accounts:
        raise ValueError('No Steam profile found. Sign into desktop Steam, launch stock D2R once, and rerun setup.')
    selected = choose('Add shortcuts for which Steam account?', [f'{name} (local ID {profile.name})' for profile, name in accounts]) if len(accounts) > 1 else 0
    profile = accounts[selected][0]
    saves = folder('D2R saved games folder', saved_games(), 'Settings.json')
    plugin = game / 'd2rloader/plugins/d2rl-vrcam.dll'
    existing = plugin.is_file()
    mode = choose('How should setup install the mod?', ['Update / repair and keep calibration and video settings', 'Full install with recommended settings (resets calibration)', 'Only configure Steam shortcuts (keep installed mod)']) if existing else 1
    if mode == 2 and not all((game / name).is_file() for name in ('D2RLoader.exe', 'ReShade64.dll', 'openxr_loader.dll', 'd2rloader/plugins/d2r_vr.ini')):
        raise ValueError('The existing mod installation is incomplete. Rerun setup and choose Update / repair.')
    options = launch_options(Path(sys.executable).resolve(), ROOT / 'steamvr/launch_windows.py', runtime)
    # Validate both Steam edits before installation or downloads.
    local = profile / 'config/localconfig.vdf'
    original_text = local.read_text(encoding='utf-8')
    old_options = steam_config.text_value(steam_config.parse_text(original_text.lstrip('\ufeff')), 'UserLocalConfigStore', 'Software', 'Valve', 'Steam', 'apps', steam_config.APP_ID, 'LaunchOptions')
    steam_config.set_launch_options(original_text, options)
    shortcut_file = profile / 'config/shortcuts.vdf'
    steam_config.update_shortcuts(shortcut_file.read_bytes() if shortcut_file.exists() else None, steam, game, connections)
    print('\nReady to set up:')
    print(f'  Headset: {headset}\n  Game: {game}\n  Steam account: {accounts[selected][1]} (local ID {profile.name})')
    print('  Shortcuts: ' + ', '.join(steam_config.SHORTCUTS[connection][0] for connection in connections))
    print('  Original D2R Play: stock desktop game. VR shortcuts: local offline play.')
    print('  Game files, saves and Steam configuration are backed up before replacement.')
    if old_options and old_options != options:
        print('  Existing D2R launch options will be replaced; a backup will retain them.')
    if mode != 2:
        print('  Required loader / ReShade components are downloaded from their official sites and checksum-checked.')
    if choose('Continue with this setup?', ['Install and configure shortcuts', 'Cancel']) == 1:
        print('Setup cancelled. No game or Steam configuration was changed.')
        return
    wait_for_closed()
    if mode != 2:
        command = [sys.executable, str(ROOT / 'steamvr/install.py'), '--game', str(game),
                   '--saves', str(saves), '--plugin', str(ROOT / 'bin/d2rl-vrcam.dll'),
                   '--openxr-loader', str(ROOT / 'bin/openxr_loader.dll'), '--deps', str(ROOT / 'deps'),
                   '--backup-dir', str(ROOT / 'setup-backups/game'), '--download-deps']
        if mode == 0:
            command.append('--preserve-settings')
        subprocess.run(command, check=True)
    wait_for_closed()
    backup = steam_config.configure(profile, steam, game, options, connections, ROOT / 'setup-backups', require_closed)
    result = {'release': manifest.get('release_tag'), 'headset': headset, 'connections': connections,
              'game': str(game), 'steam_profile': profile.name, 'steam_backup': str(backup),
              'game_backups': str(ROOT / 'setup-backups/game'), 'launch_options': options}
    (ROOT / 'setup-result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('\nSetup complete. Steam library shortcuts have been created.')
    print(f'Backups: {ROOT / "setup-backups"}')
    print('Start Steam, connect your headset and play the matching VR shortcut using an OFFLINE character.')
    print('Normal Steam Play stays stock. For multiplayer, fully exit VR and use normal Play.')
    print('First VR load asks for body calibration. Controls: CONTROLS.md. Help: INSTALL-WINDOWS.md.')
    if choose('Open Steam now?', ['Open Steam', 'Later']) == 0:
        subprocess.Popen([str(steam / 'steam.exe')], cwd=steam)


if __name__ == '__main__':
    try:
        main()
    except (KeyboardInterrupt, EOFError):
        print('\nSetup cancelled. Any completed installation backups remain in setup-backups.')
        raise SystemExit(1)
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(f'\nSetup could not finish: {error}')
        print('Keep setup-backups if it exists. See INSTALL-WINDOWS.md and Windows Security Protection History if a required file was blocked.')
        raise SystemExit(1)
