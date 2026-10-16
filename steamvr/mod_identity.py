#!/usr/bin/env python3
"""Which VR mod owns the plugin files in a game folder: D2R PCVR or D2R VR.

D2R PCVR started as a fork of D2R VR 0.152 by BodyWalkVR (MIT). Up to 0.3.15
both mods installed d2rloader/plugins/d2rl-vrcam.dll with d2r_vr.ini beside it,
so either installer replaced the other mod's files. D2R PCVR now installs
d2rl-pcvr.dll and d2r_pcvr.ini; D2R VR keeps the old names.

On an update the installer carries an earlier D2R PCVR's settings and body
calibration over from d2r_vr.ini to d2r_pcvr.ini and removes its own old files
(journaled like every other change, so a rollback puts them back). The file
name alone proves nothing: the old files count as D2R PCVR's only when this
mod's install record (d2rloader/pcvr-install.json) or one of its installation
snapshots says it installed them, and the plugin does not carry D2R VR's marks.
Files of those names that belong to D2R VR are never changed, moved or deleted;
setup says another mod is installed and stops before changing anything, since
D2RLoader would load both VR plugins into one game.
"""
from dataclasses import dataclass
import json
from pathlib import Path

MOD_ID = 'd2r-pcvr'
PLUGIN = Path('d2rloader/plugins/d2rl-pcvr.dll')
SETTINGS = Path('d2rloader/plugins/d2r_pcvr.ini')
# What D2R PCVR installed up to 0.3.15, and what D2R VR still installs.
LEGACY_PLUGIN = Path('d2rloader/plugins/d2rl-vrcam.dll')
LEGACY_SETTINGS = Path('d2rloader/plugins/d2r_vr.ini')
LEGACY_FILES = (LEGACY_PLUGIN, LEGACY_SETTINGS)
PLUGINS = (PLUGIN, LEGACY_PLUGIN)
# The same path as install.RECORD (install.py imports this module).
RECORD = Path('d2rloader/pcvr-install.json')

OTHER_MOD = 'd2r-vr'
OTHER_NAME = 'D2R VR'
# Only D2R VR's plugin carries these: its Game Link (every build since 0.157)
# and its skill icon cache folder (UTF-16).
OTHER_MARKS = (b'BodyWalk Game Link', 'd2r_vr_skill_icons'.encode('utf-16-le'))
# Only D2R PCVR's plugin carries these: its icon cache folder (UTF-16, every
# release) and, from this version on, its own plugin id.
OWN_MARKS = ('d2r_vr_icons'.encode('utf-16-le'), b'd2r-pcvr-vrcam')


def plugin_owner(path):
    """MOD_ID, OTHER_MOD, 'unknown' (neither mod's marks) or None (no file)."""
    try:
        data = Path(path).read_bytes()
    except FileNotFoundError:
        return None
    except OSError:
        # There but unreadable (locked, blocked): not provably this mod's.
        return 'unknown' if Path(path).exists() else None
    if any(mark in data for mark in OTHER_MARKS):
        return OTHER_MOD
    if any(mark in data for mark in OWN_MARKS):
        return MOD_ID
    return 'unknown'


def _manifests(game, roots):
    """This mod's snapshot manifests for this game, oldest first: those named by
    the install record, and any found in the given backup folders."""
    folders = []
    record = game / RECORD
    if record.is_file():
        try:
            names = json.loads(record.read_text(encoding='utf-8')).get('snapshots', [])
        except (OSError, ValueError, AttributeError):
            names = []
        folders += [Path(name) for name in names if isinstance(name, str) and name]
    for root in roots:
        if root is None:
            continue
        root = Path(root)
        folders += [root] if (root / 'manifest.json').is_file() else (
            [source.parent for source in root.glob('*/manifest.json')] if root.is_dir() else [])
    found = {}
    for folder in folders:
        try:
            manifest = json.loads((folder / 'manifest.json').read_text(encoding='utf-8'))
            if Path(manifest['game']).resolve() != game.resolve() or not isinstance(manifest.get('installed'), list):
                continue
        except (OSError, ValueError, KeyError, TypeError):
            continue
        found[folder.resolve()] = manifest
    return [found[path] for path in sorted(found, key=lambda path: path.name)]


def _listed(manifest, relative):
    names = {str(name).replace('\\', '/').lower() for name in manifest['installed']}
    return relative.as_posix().lower() in names


def installed_legacy_files(game, roots=()):
    """True when this mod's own records say it put d2rl-vrcam.dll / d2r_vr.ini here.

    From this version on D2R PCVR never writes those names, so once its own
    d2rl-pcvr.dll is in the folder the old names are not its files. Otherwise the
    newest snapshot that installed a plugin decides; a record whose snapshots
    cannot be read was written by an earlier release (0.3.15 or older), all of
    which used the old names."""
    if (game / PLUGIN).is_file():
        return False
    manifests = [m for m in _manifests(game, roots) if any(_listed(m, p) for p in PLUGINS)]
    if manifests:
        return _listed(manifests[-1], LEGACY_PLUGIN)
    return (game / RECORD).is_file()


@dataclass(frozen=True)
class Survey:
    ours: bool       # D2R PCVR is installed here (new names, or old names that are its own)
    legacy: bool     # its old d2rl-vrcam.dll / d2r_vr.ini are here, to carry over and remove
    other: 'str | None'   # OTHER_MOD or 'unknown': a d2rl-vrcam.dll that is not D2R PCVR's
    message: str     # what to tell the player about `other` ('' when None)


def other_mod_message(owner):
    if owner == OTHER_MOD:
        return ('Another VR mod, D2R VR, is installed in this game folder '
                '(d2rloader\\plugins\\d2rl-vrcam.dll and d2r_vr.ini are its files). '
                'D2R PCVR leaves its files alone and does not install beside it: D2RLoader would start both mods in one game. '
                'Remove D2R VR with its own uninstaller first, then run this setup again.')
    return ('This game folder has a d2rloader\\plugins\\d2rl-vrcam.dll that D2R PCVR setup did not install '
            '(no D2R PCVR installation record lists it), so it may belong to D2R VR or to a copy installed by hand. '
            'Setup leaves it alone and does not install beside it: D2RLoader would start both. '
            'Remove it with its own uninstaller (or move it out of the plugins folder if you copied it there yourself), '
            'or select the backup of your earlier D2R PCVR installation, then run this setup again.')


def survey(game, roots=()):
    """What is in this game folder, read only."""
    game = Path(game)
    new = (game / PLUGIN).is_file()
    owner = plugin_owner(game / LEGACY_PLUGIN)
    ours_legacy = owner != OTHER_MOD and installed_legacy_files(game, roots)
    legacy = ours_legacy and any((game / name).is_file() for name in LEGACY_FILES)
    other = None
    if owner is not None and not ours_legacy:
        other = OTHER_MOD if owner == OTHER_MOD else 'unknown'
    return Survey(ours=new or legacy, legacy=legacy, other=other,
                  message=other_mod_message(other) if other else '')
