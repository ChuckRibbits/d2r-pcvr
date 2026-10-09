#!/usr/bin/env python3
"""Local, reversible installation; does not change Steam's original executable."""
import argparse
import configparser
from datetime import datetime
import hashlib
import json
import os
from pathlib import Path
from pathlib import PurePosixPath
import shutil
import subprocess
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parent.parent
STEAM = Path.home() / '.local/share/Steam'
GAME = STEAM / 'steamapps/common/Diablo II Resurrected'
SAVES = STEAM / 'steamapps/compatdata/2536520/pfx/drive_c/users/steamuser/Saved Games/Diablo II Resurrected'
LOADER_HASH = '9286c6b5bff7f1043658411dbea305faa9698b6a455e6441668fc6d02c22aad7'
RESHADE_HASH = 'afe4c8f13048306307983b8b3d41d5bf00a86820440b0e57dea10950e1176445'
SDK_VERSION = '10.0.26100.9169'
SDK_HASH = '475269434dcd808a67853773272f972c3229c0e10c3ddc821290e70cc0f6904d'
COMPILER_HASH = '3bd6d7acfe8a0a84acd166fba1a537ce68bd21c8edce2bd9bb863cfa034136d4'


def require_game_closed():
    if os.name == 'nt':
        import csv
        rows = csv.reader(subprocess.check_output(
            ['tasklist', '/FO', 'CSV', '/NH'], text=True).splitlines())
        names = [row[0].lower() for row in rows if row]
    else:
        names = []
        for cmd in Path('/proc').glob('[0-9]*/cmdline'):
            try:
                first = cmd.read_bytes().split(b'\0', 1)[0].decode(errors='replace')
                names.append(first.replace('\\', '/').rsplit('/', 1)[-1].lower())
            except OSError:
                continue
    if any(name in ('d2r.exe', 'd2rloader.exe') for name in names):
        raise SystemExit('Save and exit D2R before installing or restoring the mod.')


def download_verified(url, path, digest, max_bytes=100 * 1024 * 1024):
    if path.is_file() and hashlib.sha256(path.read_bytes()).hexdigest() == digest:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    print(f'Downloading {url}')
    request = urllib.request.Request(url, headers={'User-Agent': 'D2R-PCVR-installer/0.1'})
    with urllib.request.urlopen(request, timeout=60) as response:
        data = response.read(max_bytes + 1)
    if hashlib.sha256(data).hexdigest() != digest:
        raise SystemExit(f'Checksum mismatch for {url}; no game files changed.')
    path.write_bytes(data)


def safe_relative(name):
    # Apply Windows path rules even when preparing/installing on Linux.
    path = PurePosixPath(name.replace('\\', '/'))
    if path.is_absolute() or '..' in path.parts or ':' in str(path) or not path.parts:
        raise SystemExit('Unsafe path in loader archive or backup manifest')
    return Path(*path.parts)


def default_saves(game):
    if os.name == 'nt':
        return Path.home() / 'Saved Games/Diablo II Resurrected'
    # Steam creates the Proton prefix in the library containing the game.
    return game.parent.parent / 'compatdata/2536520/pfx/drive_c/users/steamuser/Saved Games/Diablo II Resurrected'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--game', type=Path, default=GAME)
    ap.add_argument('--deps', type=Path, default=Path.home() / 'vrdeps')
    ap.add_argument('--saves', type=Path, help='Saved Games folder (for custom prefixes/redirected Windows folders)')
    ap.add_argument('--plugin', type=Path, default=ROOT / 'build-frame/d2rl-vrcam.dll')
    ap.add_argument('--openxr-loader', type=Path, help='Windows x64 openxr_loader.dll; required on native Windows')
    ap.add_argument('--backup-dir', type=Path, default=ROOT / 'build-frame/backups')
    ap.add_argument('--download-deps', action='store_true', help='Fetch pinned D2RLoader and ReShade from their official sites')
    ap.add_argument('--restore', type=Path, help='Restore the installation snapshot made by this script')
    args = ap.parse_args()
    game = args.game.resolve()
    require_game_closed()
    saves = (args.saves or default_saves(game)).resolve()
    if args.restore:
        snapshot = args.restore.resolve()
        m = json.loads((snapshot / 'manifest.json').read_text())
        if str(game) != m['game']:
            raise SystemExit('Snapshot belongs to a different game folder')
        for rel in m['installed']:
            rel = safe_relative(rel)
            target = game / rel
            old = snapshot / 'previous' / rel
            if old.exists():
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(old, target)
            elif target.is_file():
                target.unlink()
        saves = Path(m.get('saves', str(saves)))
        saves.mkdir(parents=True, exist_ok=True)
        shutil.copy2(snapshot / 'saves/Settings.json', saves / 'Settings.json')
        print('Mod files and video settings restored. Character progress was kept.')
        return
    exe = game / 'D2R.exe'
    if not exe.is_file() or '3.3.93787'.encode('utf-16-le') not in exe.read_bytes():
        raise SystemExit('This prototype requires Steam D2R 3.3.93787')
    if not args.plugin.is_file():
        raise SystemExit('Build the plugin first')
    if os.name == 'nt' and not args.openxr_loader:
        raise SystemExit('Native Windows requires --openxr-loader pointing to the release bin/openxr_loader.dll')
    if args.openxr_loader and not args.openxr_loader.is_file():
        raise SystemExit('OpenXR loader DLL was not found')
    if not (saves / 'Settings.json').is_file():
        raise SystemExit(f'Launch stock D2R once first, or pass --saves; Settings.json not found in {saves}')
    # Read and validate all inputs before changing any game files.
    settings = json.loads((saves / 'Settings.json').read_text(encoding='utf-8-sig'))
    archive = args.deps / 'D2RLoader-1.3.1-beta.zip'
    reshade = args.deps / 'reshade-6.8/ReShade64.dll'
    compiler = args.deps / 'd3dcompiler47/modern/d3dcompiler_47.dll'
    if args.download_deps:
        download_verified('https://d2rloader.net/downloads/D2RLoader-1.3.1-beta.zip', archive, LOADER_HASH)
        setup = args.deps / 'ReShade_Setup_6.8.0_Addon.exe'
        download_verified('https://reshade.me/downloads/ReShade_Setup_6.8.0_Addon.exe', setup, RESHADE_HASH)
        # Official setup executable contains an appended ZIP. Extract the DLL
        # locally; do not run setup or create an automatically loaded DXGI proxy.
        with zipfile.ZipFile(setup) as z:
            reshade.parent.mkdir(parents=True, exist_ok=True)
            reshade.write_bytes(z.read('ReShade64.dll'))
        if os.name != 'nt':
            # Wine's compiler stayed busy throughout multi-minute tests of the
            # effect. Use Microsoft's current SDK redist privately under the
            # mod; the Windows 8.1 Winetricks version lacks shader model 5.1.
            sdk = args.deps / f'd3dcompiler47/microsoft.windows.sdk.cpp.{SDK_VERSION}.nupkg'
            download_verified(f'https://api.nuget.org/v3-flatcontainer/microsoft.windows.sdk.cpp/{SDK_VERSION}/{sdk.name}',
                              sdk, SDK_HASH, max_bytes=200 * 1024 * 1024)
            with zipfile.ZipFile(sdk) as z:
                compiler.parent.mkdir(parents=True, exist_ok=True)
                compiler.write_bytes(z.read('c/Redist/D3D/x64/d3dcompiler_47.dll'))
    if os.name != 'nt' and not compiler.is_file():
        raise SystemExit('Proton needs the SDK shader compiler; run with --download-deps.')
    if compiler.is_file() and hashlib.sha256(compiler.read_bytes()).hexdigest() != COMPILER_HASH:
        raise SystemExit('SDK shader compiler checksum mismatch; run with --download-deps.')
    if hashlib.sha256(archive.read_bytes()).hexdigest() != LOADER_HASH:
        raise SystemExit('D2RLoader archive checksum mismatch')
    payload = {}
    with zipfile.ZipFile(archive) as z:
        for name in z.namelist():
            if not name.endswith('/'):
                rel = safe_relative(name)
                if rel.parts[0].lower() not in ('mods', 'd2rloader', 'd2rloader.exe', 'd2rcore.dll', 'd2rloader.mpq'):
                    raise SystemExit(f'Unexpected file in loader archive: {name}')
                payload[rel] = z.read(name)
    payload[Path('ReShade64.dll')] = reshade.read_bytes()
    payload[Path('d2rloader/plugins/d2rl-vrcam.dll')] = args.plugin.read_bytes()
    if compiler.is_file():
        payload[Path('d2rloader/runtime/d3dcompiler_47.dll')] = compiler.read_bytes()
    if args.openxr_loader:
        payload[Path('openxr_loader.dll')] = args.openxr_loader.read_bytes()
    snapshot = args.backup_dir / datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    snapshot.mkdir(parents=True)
    shutil.copytree(saves, snapshot / 'saves')
    installed = []
    manifest = {'game': str(game), 'saves': str(saves), 'installed': installed}
    (snapshot / 'manifest.json').write_text(json.dumps(manifest, indent=2))

    def put(rel, content):
        dest = game / rel
        if dest.exists():
            old = snapshot / 'previous' / rel
            old.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(dest, old)
        dest.parent.mkdir(parents=True, exist_ok=True)
        installed.append(str(rel))
        # Journal before replacing so an interrupted install can be restored.
        (snapshot / 'manifest.json').write_text(json.dumps(manifest, indent=2))
        dest.write_bytes(content)

    for rel, content in payload.items():
        put(rel, content)
    cfg = configparser.ConfigParser(interpolation=None, strict=False, allow_no_value=True)
    # Public default INI has explanatory text outside any section. Start at [camera].
    cfg.read_string('[camera]' + (ROOT / 'd2r_vr.ini').read_text(encoding='utf-8').split('[camera]', 1)[1])
    changes = {
        'camera': {'fov_from_flatvr': '0', 'neck_model': '0', 'near': '0.2'},
        'stereo': {'afr': '1', 'pair_per_tick': '1', 'true_scale': '0', 'convergence': '0', 'swap': '0'},
        'turn': {'snap': '30'},
        'mode': {'platform': '1', 'vr_mode': '4', 'vr_mode_order': '2'},
        'hud': {'hide': '0', 'map': '0', 'stereo_labels': '1', 'monster_draw_order': '1'},
        'update': {'check': '0'},
        'render': {'game_height_fog': '1', 'fixed_foveation': '1', 'foveation_inner': '0.55',
                   'symmetric_projection': '1', 'defer_gpu_wait': '0',
                   'eye_tracked_foveation': '0', 'gaze_foveation_inner': '0.35'},
    }
    for section, values in changes.items():
        if not cfg.has_section(section):
            cfg.add_section(section)
        for key, value in values.items():
            cfg.set(section, key, value)
    import io
    out = io.StringIO(); cfg.write(out)
    put(Path('d2rloader/plugins/d2r_vr.ini'), out.getvalue().encode())
    put(Path('ReShade.ini'), b'[GENERAL]\nEffectSearchPaths=.\\reshade-shaders\\Shaders\nTextureSearchPaths=.\\reshade-shaders\\Textures\nPresetPath=.\\D2R_Frame_ReShade.ini\n[INPUT]\nKeyOverlay=36,0,0,0\n')
    put(Path('D2R_Frame_ReShade.ini'), b'Techniques=D2R_DepthFog@D2R_DepthFog.fx\n')
    put(Path('reshade-shaders/Shaders/D2R_DepthFog.fx'), (ROOT / 'reshade/D2R_DepthFog.fx').read_bytes())
    for texture in (ROOT / 'reshade/sky').glob('*.png'):
        put(Path('D2R_Sky_ours') / texture.name, texture.read_bytes())
        put(Path('reshade-shaders/Textures/D2R_Sky_ours') / texture.name, texture.read_bytes())
    settings.update({'VSync': 0, 'NVIDIA DLSS': 0, 'Anti Aliasing': 1, 'Framerate Cap': 0,
                     'Dynamic Resolution Scaling': 0, 'Camera Shake': 0,
                     'Resolution Scale': 80, 'Ambient Occlusion Quality': 2,
                     'Atmospherics Quality': 2, 'Shadow Quality': 2})
    (saves / 'Settings.json').write_text(json.dumps(settings, indent=4))
    print(f'Installed offline VR prototype. Save and file backup: {snapshot}')


if __name__ == '__main__':
    main()
