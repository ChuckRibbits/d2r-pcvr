"""Bounded, read-only D2R potion mesh/texture adapter; no game assets shipped.

Reads little-endian Granny 7/64-bit data with side-table pointer relocations.
BitKnit decoder: vendor/bitknit.py (MIT). Only static potion geometry is exported;
no animation, skeleton or native code is executed.
"""
import math
import struct
from types import SimpleNamespace
from vendor.bitknit import decompress_bitknit

LIMIT = 8 * 1024 * 1024


def span(data, offset, size):
    if offset < 0 or size < 0 or offset + size > len(data):
        raise ValueError('Asset range out of bounds')
    return data[offset:offset + size]


def unpack(fmt, data, offset=0):
    return struct.unpack(fmt, span(data, offset, struct.calcsize(fmt)))


def decode(data, size, compression):
    if not 0 <= size <= LIMIT:
        raise ValueError('Oversized Granny section')
    if not size:
        return b''
    if compression == 0:
        return span(data, 0, size)
    if compression != 4:
        raise ValueError('Unsupported Granny compression')
    section = SimpleNamespace(index=0, compression=4, expanded_size=size,
                              data_size=len(data), first_16bit=size, first_8bit=size, compression_name='BitKnit2')
    result = decompress_bitknit(section, data)
    if len(result) != size:
        raise ValueError('Truncated Granny section')
    return result


class Granny:
    def __init__(self, data):
        if len(data) > LIMIT or data[:16] != bytes.fromhex('e59b495e6f631f141e13eba990beedc4'):
            raise ValueError('Expected LE64 Granny file')
        version, size, _, offset, count, ts, to, rs, ro = unpack('<9I', data, 32)
        if version != 7 or size != len(data) or not 1 <= count <= 16:
            raise ValueError('Invalid Granny header')
        self.sections, self.pointers, self.types = [], {}, {}
        expanded = 0
        for i in range(count):
            comp, start, length, out, _, _, _, fix, fixes, _, _ = unpack('<11I', data, 32 + offset + i * 44)
            expanded += out
            if expanded > LIMIT or fixes > 20000:
                raise ValueError('Oversized Granny data')
            self.sections.append(decode(span(data, start, length), out, comp))
            if fixes:
                if comp == 4:
                    packed, = unpack('<I', data, fix)
                    reloc = decode(span(data, fix + 4, packed), fixes * 12, 4)
                else:
                    reloc = span(data, fix, fixes * 12)
                for src, sector, dest in struct.iter_unpack('<3I', reloc):
                    if src + 8 > out or sector >= count:
                        raise ValueError('Invalid Granny relocation')
                    self.pointers[i, src] = (sector, dest)
        self.root = self.fields((rs, ro), (ts, to))

    def read(self, ref, fmt):
        return unpack(fmt, self.sections[ref[0]], ref[1])

    def ptr(self, ref, offset=0):
        return self.pointers.get((ref[0], ref[1] + offset))

    def string(self, ref):
        if ref is None:
            return ''
        data = self.sections[ref[0]]
        end = data.find(b'\0', ref[1], min(len(data), ref[1] + 1024))
        if end < ref[1]:
            raise ValueError('Invalid Granny string')
        return data[ref[1]:end].decode('utf-8')

    def definition(self, ref):
        if ref not in self.types:
            fields, offset = [], 0
            for i in range(128):
                at = (ref[0], ref[1] + i * 44)
                kind, = self.read(at, '<I')
                if not kind:
                    break
                width, = self.read((at[0], at[1] + 20), '<I')
                widths = {2: 8, 3: 12, 4: 12, 5: 16, 7: 20, 8: 8, 9: 68,
                          10: 4, 11: 1, 12: 1, 13: 1, 14: 1, 15: 2, 16: 2,
                          17: 2, 18: 2, 19: 4, 20: 4, 21: 2, 22: 8}
                if kind not in widths or width > 64:
                    raise ValueError('Unsupported Granny field')
                fields.append((self.string(self.ptr(at, 4)), kind, self.ptr(at, 12), max(1, width), offset))
                offset += widths[kind] * max(1, width)
            else:
                raise ValueError('Unterminated Granny type')
            self.types[ref] = fields, offset
        return self.types[ref]

    def fields(self, ref, type_ref):
        if ref is None or type_ref is None:
            raise ValueError('Missing Granny reference')
        return {name: (kind, sub, width, (ref[0], ref[1] + offset))
                for name, kind, sub, width, offset in self.definition(type_ref)[0]}

    def reference(self, field):
        kind, sub, _, ref = field
        if kind != 2:
            raise ValueError('Expected Granny reference')
        return self.fields(self.ptr(ref), sub)

    def array(self, field):
        kind, sub, _, ref = field
        if kind == 7:
            sub = self.ptr(ref)
            ref = (ref[0], ref[1] + 8)
        elif kind not in (3, 4):
            raise ValueError('Expected Granny array')
        count, = self.read(ref, '<I')
        if not 0 <= count <= 65536:
            raise ValueError('Oversized Granny array')
        base = self.ptr(ref, 4)
        if count and base is None:
            raise ValueError('Missing Granny array data')
        stride = 8 if kind == 4 else self.definition(sub)[1]
        refs = [(base[0], base[1] + i * stride) for i in range(count)]
        return [self.fields(self.ptr(r) if kind == 4 else r, sub) for r in refs]

    def scalar(self, field):
        kind, _, width, ref = field
        formats = {10:'f', 13:'b', 14:'B', 15:'h', 16:'H', 17:'h', 18:'H', 19:'i', 20:'I', 21:'e'}
        if kind not in formats:
            raise ValueError('Unsupported geometry component')
        values = self.read(ref, '<' + formats[kind] * width)
        divisor = {13:127., 14:255., 17:32767., 18:65535.}.get(kind, 1)
        return tuple(v / divisor for v in values) if divisor != 1 else values

    def meshes(self):
        result = []
        for mesh in self.array(self.root['Meshes']):
            vertices = self.array(self.reference(mesh['PrimaryVertexData'])['Vertices'])
            topo = self.reference(mesh['PrimaryTopology'])
            indices = self.array(topo['Indices16']) or self.array(topo['Indices'])
            ix = [self.scalar(next(iter(i.values())))[0] for i in indices]
            data = [(*self.scalar(v['Position'])[:3], *self.scalar(v['Normal'])[:3],
                     *self.scalar(v['TextureCoordinates0'])[:2]) for v in vertices]
            if not data or not ix or len(ix) % 3 or min(ix) < 0 or max(ix) >= len(data):
                raise ValueError('Invalid mesh topology')
            if any(len(v) != 8 or not all(math.isfinite(x) for x in v) for v in data):
                raise ValueError('Invalid mesh vertices')
            result.append((self.string(self.ptr(mesh['Name'][3])), data, ix))
        if not 1 <= len(result) <= 64:
            raise ValueError('Unexpected potion mesh count')
        return result


def texture(data):
    """Return original BC mip payloads for direct GPU sampling, no recompression.

    D2R texture header/mip offsets: TextureReader in the MIT D2R-Reimagined
    level editor; 61/62 are BC3 and offsets are relative to their own field.
    """
    magic, fmt, _, width, height = unpack('<IHHII', data)
    mips, = unpack('<I', data, 28)
    if magic != 0x2845443c or fmt not in (57, 58, 61, 62, 31) or not 1 <= mips <= 14:
        raise ValueError('Unsupported potion texture')
    if not 1 <= width <= 4096 or not 1 <= height <= 4096:
        raise ValueError('Invalid potion texture size')
    payloads = []
    for level in range(mips):
        size, relative = unpack('<II', data, 36 + level * 8)
        w, h = max(1, width >> level), max(1, height >> level)
        expected = w * h * 4 if fmt == 31 else ((w + 3)//4) * ((h + 3)//4) * (8 if fmt in (57,58) else 16)
        offset = 40 + level * 8 + relative
        if size != expected or offset < 36 + mips * 8:
            raise ValueError('Invalid potion texture mip')
        payloads.append(span(data, offset, size))
    # DXGI SRGB formats; decoding takes place during texture sampling.
    return width, height, {57:72,58:72,61:78,62:78,31:29}[fmt], payloads
