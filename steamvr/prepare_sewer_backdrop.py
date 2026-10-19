"""Prepare a visual-only Kurast tunnel backdrop in the isolated pcvr-dev mod.

Presets and geometry measurements come from the player's own archives. Generated
overrides stay local. The backdrop references the game's existing black quad;
no model, texture, collision, warp or character data is distributed or edited.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import tempfile

from game_assets import open_assets
from game_install import validate_game
from install import require_game_closed
from potion_assets import Granny
from prepare_dev_scenarios import MOD_INFO

PRESET_DIR = 'data/hd/env/preset/act3/kurast/'
PRESETS = tuple(PRESET_DIR + name + '.json' for name in
                ('burbssewer0', 'burbssewer1', 'metrosewer0', 'metrosewer1'))
STAIRS = 'data/hd/env/model/act3/kurast/act3_kurast_stairs/stairs_sewers01'
BLACK = 'data/hd/env/model/act4/fort/act4_fort_stairs/black_square01'
RECEIPT = 'pcvr-sewer-backdrop-preview.json'
PREFIX = 'PCVR_Sewer_Backdrop_'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def mul(v, scale):
    return tuple(x * scale for x in v)


def rotate(v, q):
    x, y, z, w = q
    tx, ty, tz = 2 * (y*v[2] - z*v[1]), 2 * (z*v[0] - x*v[2]), 2 * (x*v[1] - y*v[0])
    return (v[0] + w*tx + y*tz - z*ty, v[1] + w*ty + z*tx - x*tz,
            v[2] + w*tz + x*ty - y*tx)


def product(a, b):
    x, y, z, w = a
    u, v, t, s = b
    return (w*u+x*s+y*t-z*v, w*v-x*t+y*s+z*u,
            w*t+x*v-y*u+z*s, w*s-x*u-y*v-z*t)


def vector(value, keys='xyz'):
    try:
        result = tuple(float(value[k]) for k in keys)
    except (KeyError, TypeError, ValueError) as error:
        raise ValueError('Invalid native transform') from error
    if not all(math.isfinite(x) and abs(x) < 1e6 for x in result):
        raise ValueError('Nonfinite or oversized transform')
    return result


def fields(granny, obj):
    data = obj['ExtendedData']
    return granny.fields(granny.ptr(data[3], 8), granny.ptr(data[3]))


def translation(granny):
    models = granny.array(granny.root['Models'])
    if len(models) != 1:
        raise ValueError('Expected one static native model')
    values = granny.read(models[0]['InitialPlacement'][3], '<I3f4f9f')
    identity = (1., 0., 0., 0., 1., 0., 0., 0., 1.)
    if values[0] & ~1 or tuple(values[4:8]) != (0., 0., 0., 1.) or tuple(values[8:]) != identity:
        raise ValueError('Unexpected native model placement')
    return tuple(values[1:4]) if values[0] & 1 else (0., 0., 0.)


def geometry(assets):
    """Measure the native lower floor edge and the reusable black rectangle."""
    raw_stairs, raw_black = (assets.read(path + '_lod4.model') for path in (STAIRS, BLACK))
    stairs, black = Granny(raw_stairs), Granny(raw_black)
    meshes = stairs.array(stairs.root['Meshes'])
    if len(meshes) != 1:
        raise ValueError('Unexpected sewer mesh layout')
    mesh = meshes[0]
    scale = stairs.scalar(fields(stairs, mesh)['VertexScale'])[0]
    decoded = stairs.meshes()
    if len(decoded) != 1 or not 1 < scale < 100:
        raise ValueError('Unexpected sewer geometry scale')
    _, vertices, indices = decoded[0]
    placement = translation(stairs)
    positions = [add(mul(v[:3], scale), placement) for v in vertices]
    materials = [stairs.string(stairs.ptr(stairs.reference(b['Material'])['Name'][3]))
                 for b in stairs.array(mesh['MaterialBindings'])]
    floor = set()
    topology = stairs.reference(mesh['PrimaryTopology'])
    for group in stairs.array(topology['Groups']):
        material = int(stairs.scalar(group['MaterialIndex'])[0])
        start, count = (int(stairs.scalar(group[k])[0]) for k in ('TriFirst', 'TriCount'))
        if not 0 <= material < len(materials) or start < 0 or count <= 0 or (start+count)*3 > len(indices):
            raise ValueError('Invalid sewer triangle group')
        if materials[material] == 'terrain':
            floor.update(indices[start*3:(start+count)*3])
    if len(floor) < 4:
        raise ValueError('Native sewer floor not identified')
    lowest = min(positions[i][1] for i in floor)
    end = sorted(set(tuple(round(x, 4) for x in positions[i]) for i in floor
                     if abs(positions[i][1] - lowest) < .01))
    if len(end) != 2:
        raise ValueError('Expected a single open sewer floor edge')
    delta = tuple(end[1][i] - end[0][i] for i in range(3))
    width = math.hypot(delta[0], delta[2])
    if not 2 < width < 20 or abs(delta[1]) > .05:
        raise ValueError('Unexpected sewer floor edge')
    right = (delta[0]/width, 0., delta[2]/width)
    normal = (-right[2], 0., right[0])
    middle = mul(add(*end), .5)
    top = max((positions[i] for i in floor), key=lambda p: p[1])
    if sum(normal[i] * (top[i]-middle[i]) for i in range(3)) <= 0:
        raise ValueError('Sewer floor direction changed')
    upper = max(p[1] for p in positions)
    height = upper - lowest + 1.
    if not 2 < height < 20:
        raise ValueError('Unexpected sewer opening height')
    # The lowest floor edge is narrower than the visible opening. The first
    # headset preview left a strip exposed at the right jamb: overlap that
    # side farther, in stair-local coordinates rather than screen coordinates.
    left_overlap, right_overlap = 1., 3.
    # Half a world unit behind the open floor end, with edges hidden in stone.
    center = add(middle, mul(normal, -.5))
    center = add(center, mul(right, (right_overlap-left_overlap)/2))
    center = (center[0], (lowest+upper)/2, center[2])
    bmeshes = black.array(black.root['Meshes'])
    decoded = black.meshes()
    if len(bmeshes) != 1 or len(decoded) != 1:
        raise ValueError('Unexpected black rectangle layout')
    bscale = black.scalar(fields(black, bmeshes[0])['VertexScale'])[0]
    _, vs, ix = decoded[0]
    if len(vs) != 4 or len(ix) != 6:
        raise ValueError('Black backdrop is no longer a two-triangle rectangle')
    points = [add(mul(v[:3], bscale), translation(black)) for v in vs]
    lo = tuple(min(p[i] for p in points) for i in range(3))
    hi = tuple(max(p[i] for p in points) for i in range(3))
    if hi[1]-lo[1] > .001 or not 90 < hi[0]-lo[0] < 110 or not 90 < hi[2]-lo[2] < 110:
        raise ValueError('Unexpected native black rectangle bounds')
    half = math.sqrt(.5)
    yaw = -math.atan2(right[2], right[0])
    facing = product((0., math.sin(yaw/2), 0., math.cos(yaw/2)), (half, 0., 0., half))
    return dict(center=center, facing=facing,
                scale=((width+left_overlap+right_overlap)/(hi[0]-lo[0]), 1., height/(hi[2]-lo[2])),
                quad_center=mul(add(lo, hi), .5),
                source_sha256={STAIRS: digest(raw_stairs), BLACK: digest(raw_black)})


def backdrop_entities(preset, measured):
    if not isinstance(preset, dict) or preset.get('type') != 'Preset':
        raise ValueError('Expected a Kurast preset')
    result = copy.deepcopy(preset)
    entities = result.get('entities')
    dependencies = result.get('dependencies', {}).get('models')
    if not isinstance(entities, list) or not isinstance(dependencies, list):
        raise ValueError('Unexpected native preset layout')
    if any(str(e.get('name', '')).startswith(PREFIX) for e in entities):
        raise ValueError('Backdrop already present in source preset')
    candidates = [e for e in entities if any(c.get('type') == 'ModelDefinitionComponent' and
                  c.get('filename', '').replace('\\', '/').lower() == STAIRS + '.model'
                  for c in e.get('components', []))]
    if len(candidates) != 1:
        raise ValueError('Expected exactly one sewer stairwell')
    transforms = [c for c in candidates[0]['components'] if c.get('type') == 'TransformDefinitionComponent']
    if len(transforms) != 1:
        raise ValueError('Expected one sewer transform')
    transform = transforms[0]
    position, orientation, scale = (vector(transform[k], 'xyzw' if k == 'orientation' else 'xyz')
                                    for k in ('position', 'orientation', 'scale'))
    if abs(sum(x*x for x in orientation)-1) > .001 or not .01 < scale[0] < 100 or any(abs(x-scale[0]) > .001 for x in scale):
        raise ValueError('Unsupported sewer transform scale/rotation')
    existing = {e['id'] for e in entities}
    for side, flip in (('front', (0., 0., 0., 1.)), ('back', (1., 0., 0., 0.))):
        name = PREFIX + side
        identifier = int.from_bytes(hashlib.sha256(name.encode()).digest()[:4], 'little')
        if identifier in existing:
            raise ValueError('Backdrop entity ID collision')
        facing = product(orientation, product(measured['facing'], flip))
        size = mul(measured['scale'], scale[0])
        center = add(position, rotate(mul(measured['center'], scale[0]), orientation))
        offset = rotate(tuple(measured['quad_center'][i]*size[i] for i in range(3)), facing)
        location = tuple(center[i]-offset[i] for i in range(3))
        entities.append(dict(type='Entity', name=name, id=identifier, components=[
            dict(type='TransformDefinitionComponent', name=name+'_transform',
                 position=dict(zip('xyz', location)), orientation=dict(zip('xyzw', facing)),
                 scale=dict(zip('xyz', size)), inheritOnlyPosition=False),
            dict(type='ModelDefinitionComponent', name=name+'_model', filename=BLACK+'.model',
                 visibleLayers=1, lightMask=0, shadowMask=0, ghostShadows=False, floorModel=False,
                 terrainBlendEnableYUpBlend=False, terrainBlendMode=0)]))
        existing.add(identifier)
    if not any(m.get('path') == BLACK+'.model' for m in dependencies):
        dependencies.append({'path': BLACK+'.model'})
    return result


def generate(assets):
    measured = geometry(assets)
    output = {}
    for name in PRESETS:
        raw = assets.read(name)
        if len(raw) > 16*1024*1024:
            raise ValueError('Oversized native sewer preset')
        changed = backdrop_entities(json.loads(raw), measured)
        output[name] = (json.dumps(changed, ensure_ascii=False, separators=(',', ':'))+'\n').encode('utf-8')
    return output, measured


def checked_path(root, name):
    if name not in (*PRESETS, RECEIPT):
        raise ValueError('Unexpected backdrop destination')
    path = root / name
    if root.resolve() != root or path.resolve() != path:
        raise ValueError('Backdrop destination must not contain links')
    return path


def checked_root(game):
    root = Path(game).resolve() / 'mods/pcvr-dev/pcvr-dev.mpq'
    if root.resolve() != root or (root/'modinfo.json').resolve() != root/'modinfo.json' or (root/'modinfo.json').read_text(encoding='utf-8') != MOD_INFO:
        raise ValueError('Requires the existing isolated pcvr-dev metadata/save path')
    return root


def install_preview(root, payload, measured):
    if set(payload) != set(PRESETS):
        raise ValueError('Incomplete backdrop payload')
    for name in (*PRESETS, RECEIPT):
        if checked_path(root, name).exists():
            raise ValueError(f'Existing override left untouched: {name}')
    record = dict(format=1, files={name: digest(data) for name, data in payload.items()}, geometry=measured)
    receipt = checked_path(root, RECEIPT)
    created = []
    # Record ownership before writing any generated game data.
    with receipt.open('x', encoding='utf-8') as stream:
        json.dump(record, stream, indent=2)
        stream.write('\n')
    try:
        for name, data in payload.items():
            path = checked_path(root, name)
            path.parent.mkdir(parents=True, exist_ok=True)
            temporary = None
            try:
                with tempfile.NamedTemporaryFile(dir=path.parent, prefix='.pcvr-sewer-', delete=False) as stream:
                    temporary = Path(stream.name)
                    stream.write(data)
                if path.exists():
                    raise ValueError(f'Existing override left untouched: {name}')
                temporary.rename(path)
                created.append(path)
            finally:
                if temporary is not None:
                    temporary.unlink(missing_ok=True)
            if digest(path.read_bytes()) != record['files'][name]:
                raise OSError('Backdrop copy verification failed')
    except BaseException:
        for path in created:
            if digest(path.read_bytes()) == record['files'][path.relative_to(root).as_posix()]:
                path.unlink()
        receipt.unlink()
        raise
    return record


def remove_preview(root):
    receipt = checked_path(root, RECEIPT)
    if not receipt.exists():
        return 0
    record = json.loads(receipt.read_text(encoding='utf-8'))
    if record.get('format') != 1 or set(record.get('files', {})) != set(PRESETS):
        raise ValueError('Unrecognized backdrop receipt')
    owned = []
    for name, expected in record['files'].items():
        path = checked_path(root, name)
        if path.exists():
            if digest(path.read_bytes()) != expected:
                raise ValueError(f'Changed override left untouched: {name}')
            owned.append(path)
    for path in owned:
        path.unlink()
    receipt.unlink()
    return len(owned)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', type=Path, required=True)
    parser.add_argument('--remove', action='store_true')
    args = parser.parse_args()
    try:
        require_game_closed()
        root = checked_root(args.game)
        if args.remove:
            print(f'Removed {remove_preview(root)} owned sewer backdrop presets.')
            return
        validate_game(args.game)
        with open_assets(args.game) as assets:
            payload, measured = generate(assets)
        require_game_closed()
        record = install_preview(checked_root(args.game), payload, measured)
        print(f'Installed {len(record["files"])} local sewer backdrop presets. Restart pcvr-dev to test.')
    except (OSError, ValueError, RuntimeError) as error:
        parser.exit(1, f'{error}\n')


if __name__ == '__main__':
    main()
