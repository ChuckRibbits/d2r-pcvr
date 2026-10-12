#!/usr/bin/env python3
"""Build the mod's local native-icon cache from this user's installed game.

Standard-library Python, Windows/Linux; Battle.net also needs CascLib on Windows.
Never distribute the resulting cache.
The generated wheel/wrist frames are separate assets embedded in the plugin.
"""
import argparse
import csv
import hashlib
import io
import json
import os
import re
import struct
import zlib
from pathlib import Path
from game_assets import open_assets, build_hash

VERSION = 2


def sprite_frame(data, frame=0):
    if len(data) < 40 or data[:4] != b'SpA1':
        raise ValueError('Invalid native sprite')
    version, frame_width, width, height = struct.unpack_from('<HHII', data, 4)
    count = struct.unpack_from('<I', data, 20)[0]
    size, components = struct.unpack_from('<II', data, 32)
    if version != 31 or components != 4:
        raise ValueError(f'Unsupported sprite pixel format {version}/{components}')
    if not count or not 0 <= frame < count:
        raise ValueError('Invalid sprite frame index/stride')
    # Some atlases pad every frame; others omit the final gutter.
    if width % count == 0:
        stride = width // count
    elif count > 1 and (width - count * frame_width) % (count - 1) == 0:
        stride = frame_width + (width - count * frame_width) // (count - 1)
    else:
        raise ValueError('Invalid sprite frame stride')
    if not 0 <= stride - frame_width <= 8 or (count - 1) * stride + frame_width > width:
        raise ValueError('Invalid sprite frame gutter')
    if not 0 < frame_width <= stride or not 0 < height <= 8192 or size != width * height * 4 or len(data) != 40 + size:
        raise ValueError('Invalid sprite dimensions/payload')
    rgba = b''.join(data[40 + (y * width + frame * stride) * 4:
                         40 + (y * width + frame * stride + frame_width) * 4] for y in range(height))
    return frame_width, height, rgba


def png(width, height, rgba):
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    rows = b''.join(b'\0' + rgba[y * width * 4:(y + 1) * width * 4] for y in range(height))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(rows, 6)) + chunk(b'IEND', b''))


def item_key(code):
    return 'i' + format(int.from_bytes(code.encode('ascii').ljust(4, b' '), 'little'), '08x')


def prepare(game, destination=None, force=False, casclib=None):
    game = Path(game)
    destination = Path(destination) if destination else game / 'd2rloader/plugins/d2r_vr_icons'
    metadata = destination / 'cache.json'
    build = build_hash(game)
    if not force and metadata.is_file():
        try:
            old = json.loads(metadata.read_text())
            if (old['version'] == VERSION and old['build'] == build
                    and (destination / 'manifest.tsv').is_file()
                    and all((destination / name).is_file() for name in old['images'])):
                return old
        except (ValueError, KeyError, OSError):
            pass
    destination.mkdir(parents=True, exist_ok=True)
    with open_assets(game, casclib) as assets:
        def table(name):
            return list(csv.DictReader(io.StringIO(assets.read(f'data/global/excel/{name}.txt').decode('utf-8-sig')), delimiter='\t'))

        def mapping(name):
            return {k.lower(): v for row in json.loads(assets.read(f'data/hd/items/{name}.json')) for k, v in row.items()}

        manifest = {}
        images = set()
        failures = []
        decoded = {}

        def add(key, path, frame=0):
            try:
                # Sprite sheets are decoded once, not once per assigned skill.
                if path not in decoded:
                    decoded[path] = assets.read(path)
                width, height, rgba = sprite_frame(decoded[path], frame)
                digest = hashlib.sha256(struct.pack('<II', width, height) + rgba).hexdigest()
                filename = digest + '.png'
                if filename not in images and not (destination / filename).is_file():
                    temporary = destination / (filename + '.tmp')
                    temporary.write_bytes(png(width, height, rgba))
                    os.replace(temporary, destination / filename)
                manifest[key] = filename
                images.add(filename)
            except (KeyError, ValueError, OSError) as error:
                failures.append({'key': key, 'asset': path, 'frame': frame, 'reason': str(error)})

        classes = {'ama': ('amazon', 'am'), 'sor': ('sorceress', 'so'), 'nec': ('necromancer', 'ne'),
                   'pal': ('paladin', 'pa'), 'bar': ('barbarian', 'ba'), 'dru': ('druid', 'dr'),
                   'ass': ('assassin', 'as'), 'war': ('warlock', 'wa')}
        descriptions = {r['skilldesc'].lower(): r for r in table('skilldesc')}
        skills = table('skills')
        owners = {r.get('skilldesc', '').lower(): r['charclass'] for r in skills if r.get('charclass') in classes}
        for skill in skills:
            desc = descriptions.get(skill.get('skilldesc', '').lower())
            if not desc or not skill.get('*Id', '').isdigit() or not desc.get('IconCel', '').isdigit():
                continue
            owner = skill.get('charclass') or owners.get(skill.get('skilldesc', '').lower(), '')
            folder, short = classes.get(owner, ('submenu', ''))
            add('s' + str(int(skill['*Id'])), f'data/hd/global/ui/spells/{folder}/{short}skillicon.sprite', int(desc['IconCel']))

        # The game's own HD dictionaries map base codes and unique/set names to
        # the correct inventory art (e.g. hp1 really is lesser_healing_potion).
        items = mapping('items')
        sprite_paths = [name for name in assets.files if name.startswith('data/hd/global/ui/items/')
                        and name.endswith('.sprite') and '.lowend.' not in name]
        by_asset = {}
        for path in sprite_paths:
            # Inventory groups insert one category above the item's asset path.
            suffix = '/'.join(path.split('/')[6:]).removesuffix('.sprite')
            by_asset.setdefault(suffix, []).append(path)

        def item_sprite(asset):
            if not asset:
                return None
            matches = by_asset.get(asset.lower().replace('\\', '/'), [])
            return matches[0] if len(matches) == 1 else None

        for code, info in items.items():
            if not 1 <= len(code) <= 4 or not code.isascii():
                continue
            path = item_sprite(info.get('asset', ''))
            if path:
                add(item_key(code), path)
        equipment = {r['code']: r for name in ('weapons', 'armor') for r in table(name) if r.get('code')}
        for table_name, dictionary, prefix in [('uniqueitems', 'uniques', 'u'), ('setitems', 'sets', 't')]:
            normalize = lambda name: re.sub('[^a-z0-9]', '', name.lower())
            names = {normalize(k): v for k, v in mapping(dictionary).items()}
            for row in table(table_name):
                if not row.get('*ID', '').isdigit():
                    continue
                info = names.get(normalize(row.get('index', '')), {})
                code = row.get('code') or row.get('item', '')
                family = equipment.get(code, {})
                tiers = [('normal', 'normcode'), ('uber', 'ubercode'), ('ultra', 'ultracode')]
                tier = next((name for name, column in tiers if family.get(column) == code), 'normal')
                path = item_sprite(info.get(tier, ''))
                if path:
                    add(prefix + str(int(row['*ID'])), path)
                # Upgraded unique weapons can use different art at each tier.
                # Keep the live base code alongside the unique/set record ID.
                if len(set(info.values())) > 1:
                    for name, column in tiers:
                        variant = family.get(column, '')
                        path = item_sprite(info.get(name, ''))
                        if variant and path:
                            add(prefix + str(int(row['*ID'])) + '_' + item_key(variant)[1:], path)

        # Fixed utility commands can use actual game UI symbols as well.
        utilities = {
            'journal': ('data/hd/global/ui/panel/hud_02/quest_button.sprite', 0),
            'swap': ('data/hd/global/ui/controller/panel/inventorypanel/v2/weaponswap.sprite', 0),
        }
        for key, (path, frame) in utilities.items():
            if path in assets.files:
                add(key, path, frame)
        if not any(key.startswith('s') for key in manifest) or not any(key.startswith('i') for key in manifest):
            raise ValueError('No native skill/item icons resolved; refusing incomplete cache')
        result = {'version': VERSION, 'build': assets.build_hash, 'icons': len(manifest),
                  'images': sorted(images), 'missing': failures}
        text = ''.join(f'{key}\t{filename}\n' for key, filename in sorted(manifest.items()))
        temp = destination / 'manifest.tsv.tmp'
        temp.write_text(text, encoding='ascii')
        os.replace(temp, destination / 'manifest.tsv')
        temp = destination / 'cache.json.tmp'
        temp.write_text(json.dumps(result, indent=2) + '\n')
        os.replace(temp, metadata)
        return result


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--game', required=True, type=Path)
    ap.add_argument('--output', type=Path)
    ap.add_argument('--force', action='store_true')
    ap.add_argument('--casclib', type=Path, help='Packaged Unicode x64 CascLib 3.0 (Battle.net only)')
    args = ap.parse_args()
    result = prepare(args.game, args.output, args.force, args.casclib)
    print(f"Native icon cache: {result['icons']} bindings, {len(result['images'])} images, {len(result['missing'])} unsupported assets")
