"""Narrow Steam configuration edits, with backups and an exact binary round trip."""
from dataclasses import dataclass
from datetime import datetime
import copy
import os
from pathlib import Path
import re
import shutil
import struct
import tempfile
import zlib

APP_ID = '2536520'
SHORTCUTS = {
    'steamvr': ('Diablo II: Resurrected VR - SteamVR', '-applaunch 2536520 --frame-vr', 1),
    'vdxr': ('Diablo II: Resurrected VR - Virtual Desktop', '-applaunch 2536520 --frame-vr --frame-vdxr', 0),
}


def quote(value):
    return '"' + value.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n').replace('\r', '\\r').replace('\t', '\\t') + '"'


@dataclass
class Entry:
    key: str
    value: object
    start: int
    end: int
    close: int = 0


def parse_text(text):
    """Record spans so changing one launch option leaves all other text intact."""
    token = re.compile(r'\s+|//[^\r\n]*|"(?:\\.|[^"\\])*"|[{}]|[^\s{}"]+')
    tokens = []
    offset = 0
    for match in token.finditer(text):
        if match.start() != offset:
            raise ValueError('Unsupported Steam text configuration; use the manual install directions.')
        offset = match.end()
        value = match[0]
        if value.isspace() or value.startswith('//'):
            continue
        tokens.append((value, match.start(), match.end()))
    if offset != len(text):
        raise ValueError('Truncated Steam text configuration')
    index = 0

    def unquote(value):
        if not value.startswith('"'):
            return value
        return re.sub(r'\\(.)', lambda m: {'n': '\n', 'r': '\r', 't': '\t'}.get(m[1], m[1]), value[1:-1])

    def tree(nested=False):
        nonlocal index
        result = []
        while index < len(tokens):
            key, _, _ = tokens[index]
            index += 1
            if key == '}':
                if not nested:
                    raise ValueError('Unexpected Steam configuration closing brace')
                return result
            if key == '{' or index >= len(tokens):
                raise ValueError('Missing Steam configuration value')
            value, start, end = tokens[index]
            index += 1
            if value == '{':
                children = tree(True)
                result.append(Entry(unquote(key), children, start, tokens[index - 1][2], tokens[index - 1][1]))
            elif value == '}':
                raise ValueError('Missing Steam configuration value')
            else:
                result.append(Entry(unquote(key), unquote(value), start, end))
        if nested:
            raise ValueError('Unclosed Steam configuration block')
        return result
    return tree()


def child(entries, key):
    matches = [entry for entry in entries if entry.key.lower() == key.lower()]
    if len(matches) > 1:
        raise ValueError(f'Duplicate Steam configuration key: {key}; use the manual directions.')
    return matches[0] if matches else None


def text_value(entries, *keys):
    for key in keys:
        entry = child(entries, key)
        if entry is None:
            return None
        entries = entry.value
    return entries


def set_launch_options(text, options):
    keys = ['UserLocalConfigStore', 'Software', 'Valve', 'Steam', 'apps', APP_ID, 'LaunchOptions']
    entries = parse_text(text.lstrip('\ufeff'))
    # Retain a possible BOM in the original offsets.
    if text.startswith('\ufeff'):
        return '\ufeff' + set_launch_options(text[1:], options)
    parent = None
    for index, key in enumerate(keys):
        entry = child(entries, key)
        if entry is None:
            if parent is None:
                raise ValueError('Steam local profile is missing UserLocalConfigStore. Launch Steam once first.')
            fragment = quote(options)
            for missing in reversed(keys[index:]):
                fragment = quote(missing) + '\t' + fragment if missing == 'LaunchOptions' else quote(missing) + '\n{\n' + fragment + '\n}'
            return text[:parent.close] + '\n' + fragment + '\n' + text[parent.close:]
        if index == len(keys) - 1:
            if not isinstance(entry.value, str):
                raise ValueError('Invalid Steam launch options')
            return text[:entry.start] + quote(options) + text[entry.end:]
        if not isinstance(entry.value, list):
            raise ValueError(f'Invalid Steam configuration block: {key}')
        parent, entries = entry, entry.value
    raise AssertionError('Unreachable')


def decode_binary(data):
    offset = 0

    def string():
        nonlocal offset
        end = data.index(0, offset)
        result = data[offset:end].decode('utf-8', errors='surrogateescape')
        offset = end + 1
        return result

    def tree():
        nonlocal offset
        result = {}
        while True:
            kind = data[offset]
            offset += 1
            if kind == 8:
                return result
            key = string()
            if key in result:
                raise ValueError('Duplicate Steam shortcut key; refusing to rewrite')
            if kind == 0:
                value = tree()
            elif kind == 1:
                value = string()
            elif kind == 2:
                value = struct.unpack_from('<I', data, offset)[0]
                offset += 4
            else:
                raise ValueError(f'Unsupported Steam shortcut type {kind}; use the manual directions.')
            result[key] = value

    try:
        result = tree()
    except (IndexError, struct.error) as error:
        raise ValueError('Truncated Steam shortcut configuration') from error
    if offset != len(data):
        raise ValueError('Unexpected trailing Steam shortcut data')
    return result


def encode_binary(tree):
    result = bytearray()
    for key, value in tree.items():
        kind = 0 if isinstance(value, dict) else 1 if isinstance(value, str) else 2
        result.append(kind)
        result.extend(key.encode('utf-8', errors='surrogateescape') + b'\0')
        if kind == 0:
            result.extend(encode_binary(value))
        elif kind == 1:
            result.extend(value.encode('utf-8', errors='surrogateescape') + b'\0')
        else:
            result.extend(struct.pack('<I', value))
    result.append(8)
    return bytes(result)


def update_shortcuts(original, steam, game, connections):
    root = decode_binary(original) if original is not None else {'shortcuts': {}}
    if original is not None and encode_binary(root) != original:
        raise ValueError('Steam shortcuts do not round-trip exactly; use the manual directions.')
    entries = root.get('shortcuts')
    if not isinstance(entries, dict) or any(not key.isdecimal() or not isinstance(value, dict) for key, value in entries.items()):
        raise ValueError('Unexpected Steam shortcut structure')
    previous = copy.deepcopy(entries)
    for connection in connections:
        name, options, openvr = SHORTCUTS[connection]
        matches = [key for key, item in entries.items() if item.get('appname', item.get('AppName')) == name]
        if len(matches) > 1:
            raise ValueError(f'Remove duplicate library entries named {name} in Steam, then run setup again.')
        key = matches[0] if matches else str(max(map(int, entries), default=-1) + 1)
        item = entries.setdefault(key, {})
        exe = '"' + str(steam / 'steam.exe') + '"'
        item.pop('AppName', None)
        item.update({
            'appid': zlib.crc32((exe + name).encode('utf-8')) | 0x80000000,
            'appname': name, 'Exe': exe, 'StartDir': '"' + str(steam) + '"',
            'icon': str(game / 'D2R.exe'), 'ShortcutPath': '', 'LaunchOptions': options,
            'IsHidden': 0, 'AllowDesktopConfig': 0, 'AllowOverlay': 0,
            'OpenVR': openvr, 'Devkit': 0, 'DevkitGameID': '', 'DevkitOverrideAppID': 0, 'FlatpakAppID': '',
        })
        item.setdefault('LastPlayTime', 0)
        item.setdefault('tags', {'0': 'D2R PCVR'})
    targets = {SHORTCUTS[connection][0] for connection in connections}
    for key, value in previous.items():
        if value.get('appname', value.get('AppName')) not in targets and entries[key] != value:
            raise ValueError('An unrelated Steam shortcut changed')
    result = encode_binary(root)
    if decode_binary(result) != root:
        raise ValueError('Steam shortcut verification failed')
    return result


def configure(profile, steam, game, options, connections, backups, require_closed):
    """Stage both edits and roll back a partial write; never edit a live Steam client."""
    config = profile / 'config'
    local = config / 'localconfig.vdf'
    shortcuts = config / 'shortcuts.vdf'
    require_closed()
    originals = {local: local.read_bytes(), shortcuts: shortcuts.read_bytes() if shortcuts.exists() else None}
    updates = {local: set_launch_options(originals[local].decode('utf-8'), options).encode('utf-8'),
               shortcuts: update_shortcuts(originals[shortcuts], steam, game, connections)}
    backup = backups / ('steam-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
    backup.mkdir(parents=True)
    for path, original in originals.items():
        if original is not None:
            shutil.copy2(path, backup / path.name)
    staged, replaced = {}, []
    try:
        for path, payload in updates.items():
            with tempfile.NamedTemporaryFile(dir=config, prefix='d2r-setup-', delete=False) as output:
                output.write(payload)
                staged[path] = Path(output.name)
        require_closed()
        for path, original in originals.items():
            current = path.read_bytes() if path.exists() else None
            if current != original:
                raise ValueError('Steam configuration changed during setup. Close Steam and try again.')
        for path, temporary in staged.items():
            os.replace(temporary, path)
            replaced.append(path)
        if any(path.read_bytes() != payload for path, payload in updates.items()):
            raise OSError('Steam configuration verification failed')
    except BaseException:
        for path in reversed(replaced):
            if originals[path] is None:
                path.unlink(missing_ok=True)
            else:
                path.write_bytes(originals[path])
        raise
    finally:
        for temporary in staged.values():
            temporary.unlink(missing_ok=True)
    return backup
