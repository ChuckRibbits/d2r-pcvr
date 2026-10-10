#!/usr/bin/env python3
"""Cache original 3D potion meshes and textures from the installed Steam game.

Standard-library only; works on Linux and Windows. Cache contains Blizzard
assets and stays in the user's installation, never in the mod's release.
"""
import argparse
import hashlib
import json
import os
import struct
from pathlib import Path
from steam_assets import SteamAssets
from potion_assets import Granny, texture

VERSION = 1
CODES = tuple(f'{kind}p{tier}' for kind in 'hm' for tier in range(1, 6)) + ('rvs','rvl','yps','wms','vps')


def encode(meshes, tex):
    vertices, indices = [], []
    for name, verts, ix in meshes:
        indices.extend(i + len(vertices) for i in ix)
        vertices.extend((*v, 1. if 'glass' in name.lower() else 0.) for v in verts)
    ymin, ymax = min(v[1] for v in vertices), max(v[1] for v in vertices)
    if ymax - ymin < .01 or len(vertices) > 32768 or len(indices) > 131072:
        raise ValueError('Unsupported potion geometry')
    # Authored +Y is the bottle axis. Center the neck over the grip; consistent
    # 18 cm height gives every asset the same mouth anchor used by interactions.
    top = [v for v in vertices if v[1] >= ymax - (ymax-ymin)*.025]
    x, z = (sum(v[a] for v in top) / len(top) for a in (0, 2))
    scale = .18 / (ymax - ymin)
    normalized = [((v[0]-x)*scale, (v[1]-(ymin+ymax)*.5)*scale, (v[2]-z)*scale, *v[3:]) for v in vertices]
    if any(abs(v[a]) > .16 for v in normalized for a in (0,2)):
        raise ValueError('Unexpected potion width')
    w,h,fmt,mips=tex
    result = bytearray(struct.pack('<8I', 0x31525650, VERSION, len(vertices), len(indices), w,h,fmt,len(mips)))
    for v in normalized:
        result.extend(struct.pack('<9f', *v))
    result.extend(struct.pack('<' + 'I'*len(indices), *indices))
    for mip in mips:
        result.extend(struct.pack('<I',len(mip)));result.extend(mip)
    return bytes(result)


def prepare(game, destination=None, force=False):
    game=Path(game)
    destination=Path(destination) if destination else game/'d2rloader/plugins/d2r_vr_potions'
    destination.mkdir(parents=True,exist_ok=True)
    metadata=destination/'cache.json'
    with SteamAssets(game) as assets:
        if not force and metadata.is_file():
            try:
                previous=json.loads(metadata.read_text())
                if previous['version']==VERSION and previous['build']==assets.build_hash and all(
                    hashlib.sha256((destination/(code+'.pvr')).read_bytes()).hexdigest()==previous['sha256'][code] for code in CODES):
                    return previous
            except (ValueError,KeyError,OSError):
                pass
        dictionary={k:v for row in json.loads(assets.read('data/hd/items/items.json')) for k,v in row.items()}
        output, sources={},{}
        for code in CODES:
            asset='data/hd/items/misc/'+dictionary[code]['asset']
            unit=json.loads(assets.read(asset+'.json'))
            model=unit['dependencies']['models'][0]['path'].lower().replace('.model','_lod0.model')
            albedo=next(t['path'].lower() for t in unit['dependencies']['textures'] if t['path'].lower().endswith('_alb.texture'))
            output[code]=encode(Granny(assets.read(model)).meshes(),texture(assets.read(albedo)))
            sources[code]={'model':model,'albedo':albedo}
        # Decode/validate all assets before replacing any live cache files.
        result={'version':VERSION,'build':assets.build_hash,'models':len(output),'sources':sources,
                'sha256':{code:hashlib.sha256(data).hexdigest() for code,data in output.items()}}
        for code,data in output.items():
            temp=destination/(code+'.pvr.tmp');temp.write_bytes(data);os.replace(temp,destination/(code+'.pvr'))
        temp=destination/'cache.json.tmp';temp.write_text(json.dumps(result,indent=2)+'\n');os.replace(temp,metadata)
        return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game',required=True,type=Path)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--force',action='store_true')
    args=parser.parse_args()
    result=prepare(args.game,args.output,args.force)
    print(f"Native potion cache: {result['models']} original 3D models with mipmapped textures")
