#!/usr/bin/env python3
"""Package only distributable preview files; exclude local logs, saves and dumps."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build', type=Path, default=ROOT / 'build-frame')
    parser.add_argument('--openxr-loader', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'build-frame/releases/d2r-pcvr-preview.zip')
    args = parser.parse_args()
    if subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT).strip():
        raise SystemExit('Commit source before packaging a release')
    commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    # Explicit allowlist. Never glob an install, backup, build or diagnostic tree.
    paths = ['INSTALL.md', 'HANDOFF.md', 'THIRD_PARTY.md', 'LICENSE', 'd2r_vr.ini',
             'reshade/D2R_DepthFog.fx', 'third_party/minhook/LICENSE.txt',
             'external/reshade/LICENSE.md', 'steamvr/install.py',
             'steamvr/launch.sh', 'steamvr/run-proton.sh', 'steamvr/session.py',
             'steamvr/launch_windows.py', 'steamvr/README.md', 'steamvr/TASKS.md']
    paths += ['steamvr/prepare_icons.py', 'steamvr/steam_assets.py']
    paths += ['steamvr/prepare_potions.py', 'steamvr/potion_assets.py', 'steamvr/vendor/bitknit.py']
    files = {path: ROOT / path for path in paths}
    for folder, pattern in [('reshade/sky', '*.png'), ('steamvr/licenses', '*.txt')]:
        for path in sorted((ROOT / folder).glob(pattern)):
            files[path.relative_to(ROOT).as_posix()] = path
    files['bin/d2rl-vrcam.dll'] = args.build / 'd2rl-vrcam.dll'
    files['bin/openxr-probe.exe'] = args.build / 'openxr-probe.exe'
    files['bin/openxr_loader.dll'] = args.openxr_loader
    manifest = {'source_commit': commit, 'game_version': '3.3.93787',
                'created_utc': datetime.now(timezone.utc).isoformat(),
                'verified': 'Linux/Proton Experimental/SteamVR/Steam Frame',
                'untested': ['Quest 3', 'WiVRn', 'native Windows', 'Virtual Desktop'],
                'known_issues': ['doubled creature labels', 'residual hitches', 'unfinished belt/HUD'],
                'sha256': {name: hashlib.sha256(path.read_bytes()).hexdigest() for name, path in files.items()}}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, 'w', zipfile.ZIP_DEFLATED) as z:
        for name, path in files.items():
            z.write(path, f'd2r-pcvr-preview/{name}')
        z.writestr('d2r-pcvr-preview/release-manifest.json', json.dumps(manifest, indent=2) + '\n')
    digest = hashlib.sha256(args.output.read_bytes()).hexdigest()
    args.output.with_suffix('.zip.sha256').write_text(f'{digest}  {args.output.name}\n')
    print(f'{args.output} ({args.output.stat().st_size} bytes), source {commit}')


if __name__ == '__main__':
    main()
