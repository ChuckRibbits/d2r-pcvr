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
    parser.add_argument('--variant', choices=['current-source', 'linux-baseline'], default='current-source')
    parser.add_argument('--plugin-source-note', help='Required provenance for an archived baseline DLL')
    parser.add_argument('--release-tag', help='Published release tag recorded in the manifest')
    parser.add_argument('--platform', choices=['native-windows', 'linux-proton'], default='native-windows')
    args = parser.parse_args()
    if args.variant == 'linux-baseline' and not args.plugin_source_note:
        parser.error('--variant linux-baseline requires --plugin-source-note')
    if subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT).strip():
        raise SystemExit('Commit source before packaging a release')
    commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    # Explicit allowlist. Never glob an install, backup, build or diagnostic tree.
    paths = ['INSTALL.md', 'INSTALL-WINDOWS.md', 'INSTALL-LINUX.md', 'README.md', 'RELEASE_NOTES.md',
             'Launch-VR-SteamVR.cmd', 'Launch-VR-VDXR.cmd',
             'HANDOFF.md', 'WINDOWS_HANDOFF.md', 'THIRD_PARTY.md', 'LICENSE', 'd2r_vr.ini',
             'reshade/D2R_DepthFog.fx', 'third_party/minhook/LICENSE.txt',
             'external/reshade/LICENSE.md', 'steamvr/install.py',
             'steamvr/launch.sh', 'steamvr/run-proton.sh', 'steamvr/session.py',
             'steamvr/launch_windows.py', 'steamvr/README.md', 'steamvr/TASKS.md',
             'steamvr/GAMEPLAY.md', 'steamvr/INTERACTIONS.md', 'steamvr/PERFORMANCE.md',
             'steamvr/profile_report.py', 'steamvr/stream_report.py',
             'steamvr/QUEST_TESTING.md',
             'steamvr/evidence/2026-10-10-handoff.json',
             'steamvr/evidence/2026-10-10-quest3-steamlink.json',
             'steamvr/evidence/2026-10-10-quest3-vdxr.json',
             'steamvr/evidence/2026-10-10-windows-release.json']
    paths += ['steamvr/prepare_icons.py', 'steamvr/steam_assets.py']
    paths += ['steamvr/prepare_potions.py', 'steamvr/potion_assets.py', 'steamvr/vendor/bitknit.py']
    files = {path: ROOT / path for path in paths}
    for folder, pattern in [('reshade/sky', '*.png'), ('steamvr/licenses', '*.txt')]:
        for path in sorted((ROOT / folder).glob(pattern)):
            files[path.relative_to(ROOT).as_posix()] = path
    files['bin/d2rl-vrcam.dll'] = args.build / 'd2rl-vrcam.dll'
    files['bin/openxr-probe.exe'] = args.build / 'openxr-probe.exe'
    files['bin/openxr_loader.dll'] = args.openxr_loader
    hashes = {name: hashlib.sha256(path.read_bytes()).hexdigest() for name, path in files.items()}
    validation = json.loads(files['steamvr/evidence/2026-10-10-windows-release.json'].read_text())
    validated_release = (args.variant == 'current-source' and args.platform == 'native-windows'
                         and hashes['bin/d2rl-vrcam.dll'] == validation['plugin_sha256'])
    manifest = {'source_commit': commit, 'game_version': '3.3.93787',
                'created_utc': datetime.now(timezone.utc).isoformat(),
                'release_tag': args.release_tag,
                'platform': args.platform,
                'installation_guide': 'INSTALL-WINDOWS.md' if args.platform == 'native-windows' else 'INSTALL-LINUX.md',
                'variant': args.variant,
                'plugin_source_commit': commit if args.variant == 'current-source' else None,
                'plugin_source_note': args.plugin_source_note or 'Built from current source; precise binary/headset coverage is in verification_records.',
                'verified': 'Native Windows checks and recorded predecessor headset sessions; see release_validation for scope.' if validated_release else 'Headset validation is not established for this binary by the included release record.',
                'release_validation': validation if validated_release else None,
                'verification_records': ['steamvr/evidence/2026-10-10-windows-release.json',
                                         'steamvr/evidence/2026-10-10-quest3-steamlink.json',
                                         'steamvr/evidence/2026-10-10-quest3-vdxr.json'],
                'untested': ['physical Quest 2', 'Linux/WiVRn', 'Virtual Desktop with SteamVR'],
                'known_issues': ['No controlled Windows baseline/candidate performance comparison',
                                 'Broader class, weapon and feature coverage remains incomplete',
                                 'Journal wheel cleanup has automated/preview verification, not a new headset session',
                                 'Historical Linux SteamVR streaming resets and native update stalls remain separate'],
                'sha256': hashes}
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
