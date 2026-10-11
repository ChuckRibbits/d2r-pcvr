"""Read-only local Steam D2R static-container/TVFS reader.

Format adapter based on D2RExtractor's MIT Steam reader (itself adapted from
rustydemon). See licenses/D2RExtractor-MIT.txt and THIRD_PARTY.md. No network,
game execution, decryption, archive writes, or save access.
"""
import hashlib
import re
import struct
import zlib
from pathlib import Path

MAX_FILE = 128 * 1024 * 1024


def be(data):
    return int.from_bytes(data, 'big')


def exact(stream, count):
    if not 0 <= count <= MAX_FILE:
        raise ValueError('Asset block exceeds size limit')
    data = stream.read(count)
    if len(data) != count:
        raise ValueError('Truncated asset block')
    return data


def inflate(data, limit=MAX_FILE):
    decoder = zlib.decompressobj()
    result = decoder.decompress(data, limit + 1)
    if len(result) > limit or not decoder.eof:
        raise ValueError('Invalid or oversized compressed asset')
    return result


def canonical(path):
    path = path.lower().replace('\\', '/').removeprefix('data:')
    if '..' in path.split('/'):
        raise ValueError('Invalid asset path')
    return path


def join_key(path):
    return re.sub(r'[/\\:]', '', path).lower()


class SteamAssets:
    def __init__(self, game):
        self.handles = {}
        self.files = {}
        self.digests = {}
        game = Path(game)
        config = next((game / d / '.build.config' for d in ('data', 'Data')
                       if (game / d / '.build.config').is_file()), None)
        if config is None:
            raise ValueError('Native icons currently need a Steam static-container install (.build.config)')
        self.directory = config.parent
        self.build_hash = hashlib.sha256(config.read_bytes()).hexdigest()
        kv = {}
        for line in config.read_text().splitlines():
            if '=' in line and not line.lstrip().startswith('#'):
                key, value = line.split('=', 1)
                kv[key.strip()] = value.strip()
        self.index_bits = int(kv.get('key-layout-index-bits', '0'))
        if not 0 <= self.index_bits <= 8:
            raise ValueError('Unsupported static-container layout')
        self.layouts = {int(k[11:]): tuple(map(int, v.split())) for k, v in kv.items()
                        if re.fullmatch(r'key-layout-\d+', k)}
        root = bytes.fromhex(kv['vfs-root'].split()[-1])
        self.subdirs = {bytes.fromhex(v.split()[-1])[:9]: bytes.fromhex(v.split()[-1])
                        for k, v in kv.items() if re.fullmatch(r'vfs-\d+', k)}
        raw = {}
        try:
            self._tree(self.blob(root), '', raw, 0)
            # TVFS sometimes omits separators; verified text ROOT is authoritative.
            index = raw.get('index')
            if index is None:
                raise ValueError('Steam asset index missing')
            root_data = self._spans(index)
            if hashlib.md5(root_data).hexdigest() != kv['root'].split()[0].lower():
                raise ValueError('Steam asset index checksum mismatch')
            lookup = {join_key(k): v for k, v in raw.items()}
            for line in root_data.decode('utf-8-sig').splitlines():
                fields = line.split('|')
                spans = lookup.get(join_key(fields[0]))
                if spans is not None:
                    name = canonical(fields[0])
                    self.files[name] = spans
                    if len(fields) > 1 and re.fullmatch(r'[0-9a-fA-F]{32}', fields[1]):
                        self.digests[name] = fields[1].lower()
            if len(self.files) < len(raw) // 2:
                raise ValueError('Incomplete Steam asset index')
        except Exception:
            self.close()
            raise

    def close(self):
        for handle in self.handles.values():
            handle.close()
        self.handles.clear()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    def blob(self, key):
        if len(key) != 16:
            raise ValueError('Invalid encoding key')
        bits = be(key[8:])
        at = 56 - self.index_bits
        index = (bits >> at) & ((1 << self.index_bits) - 1)
        layout = self.layouts[index]
        if len(layout) not in (3, 4) or sum(layout[:3]) > at or min(layout[:3]) < 0:
            raise ValueError('Invalid encoding-key bit layout')
        values = []
        for size in layout[:3]:
            at -= size
            values.append((bits >> at) & ((1 << size) - 1))
        chunk, archive, offset = values
        offset *= (layout[3] or 1) if len(layout) == 4 else 1
        name = f'{chunk:02d}-{archive:08x}.data'
        if name not in self.handles:
            self.handles[name] = (self.directory / name).open('rb')
        f = self.handles[name]
        f.seek(offset)
        magic = exact(f, 8)
        if magic[:1] == b'\x78':
            # Root TVFS is one raw zlib stream; stop at its actual end.
            f.seek(offset)
            decoder = zlib.decompressobj()
            result = bytearray()
            while not decoder.eof:
                data = f.read(65536)
                if not data:
                    raise ValueError('Truncated TVFS stream')
                result.extend(decoder.decompress(data, MAX_FILE + 1 - len(result)))
                if len(result) > MAX_FILE:
                    raise ValueError('TVFS exceeds size limit')
            return bytes(result)
        if magic[:4] != b'BLTE':
            raise ValueError('Unsupported asset storage encoding')
        header_size = be(magic[4:])
        header = magic + exact(f, header_size - 8)
        if header_size < 12 or header[8] != 15:
            raise ValueError('Invalid BLTE header')
        count = be(header[9:12])
        if not count or header_size != 12 + 24 * count:
            raise ValueError('Invalid BLTE block table')
        output = bytearray()
        for i in range(count):
            pos = 12 + 24 * i
            compressed, decoded = struct.unpack_from('>II', header, pos)
            data = exact(f, compressed)
            if hashlib.md5(data).digest() != header[pos + 8:pos + 24]:
                raise ValueError('BLTE block checksum mismatch')
            if data[:1] == b'N':
                block = data[1:]
            elif data[:1] == b'Z':
                block = inflate(data[1:], min(decoded, MAX_FILE))
            else:
                raise ValueError('Unsupported/encrypted asset block')
            if len(block) != decoded or len(output) + len(block) > MAX_FILE:
                raise ValueError('BLTE decoded size mismatch')
            output.extend(block)
        return bytes(output)

    def _spans(self, spans):
        size = max((offset + count for offset, count, _ in spans), default=0)
        if size > MAX_FILE:
            raise ValueError('Asset exceeds size limit')
        output = bytearray(size)
        for offset, count, key in spans:
            data = self.blob(key)
            if len(data) < count:
                raise ValueError('Truncated asset span')
            output[offset:offset + count] = data[:count]
        return bytes(output)

    def read(self, name):
        name = canonical(name)
        data = self._spans(self.files[name])
        if name in self.digests and hashlib.md5(data).hexdigest() != self.digests[name]:
            raise ValueError(f'Asset checksum mismatch: {name}')
        return data

    def _tree(self, data, prefix, files, depth):
        if depth > 64 or len(data) < 46 or data[:5] != b'TVFS\x01':
            raise ValueError('Invalid or recursive TVFS directory')
        key_size = data[6]
        if not 9 <= key_size <= 16:
            raise ValueError('Invalid TVFS key size')
        tables = []
        for at in (12, 20, 28):
            start, size = struct.unpack_from('>II', data, at)
            if start + size > len(data):
                raise ValueError('Truncated TVFS table')
            tables.append(data[start:start + size])
        path, vfs, cft = tables
        cft_bytes = max(1, (len(cft).bit_length() + 7) // 8)

        def leaf(name, offset):
            if not 0 <= offset < len(vfs):
                raise ValueError("Invalid TVFS leaf offset")
            count = vfs[offset]
            if not 1 <= count <= 224:
                raise ValueError('Invalid TVFS spans')
            at = offset + 1
            spans = []
            for _ in range(count):
                if at + 8 + cft_bytes > len(vfs):
                    raise ValueError('Truncated TVFS span')
                begin, size = struct.unpack_from('>II', vfs, at)
                location = be(vfs[at + 8:at + 8 + cft_bytes])
                if location + key_size > len(cft):
                    raise ValueError('Invalid TVFS content key')
                key = cft[location:location + key_size].ljust(16, b'\0')
                spans.append((begin, size, key))
                at += 8 + cft_bytes
            sub = self.subdirs.get(spans[0][2][:9]) if count == 1 else None
            if sub:
                self._tree(self.blob(sub), name + '/', files, depth + 1)
            else:
                files[name.replace('/', ':', 1)] = spans

        def walk(table, parent, level):
            if level > 128:
                raise ValueError('TVFS path nesting limit')
            at = 0
            name = parent
            while at < len(table):
                if table[at] == 0:
                    name += '/'
                    at += 1
                if at < len(table) and table[at] != 255:
                    length = table[at]
                    at += 1
                    if at + length > len(table):
                        raise ValueError('Truncated TVFS name')
                    name += table[at:at + length].decode('utf-8')
                    at += length
                if at < len(table) and table[at] == 0:
                    name += '/'
                    at += 1
                if at < len(table) and table[at] == 255:
                    if at + 5 > len(table):
                        raise ValueError('Truncated TVFS node')
                    node = be(table[at + 1:at + 5])
                    at += 5
                    if node & 0x80000000:
                        length = (node & 0x7fffffff) - 4
                        if not 0 <= length <= len(table) - at:
                            raise ValueError('Invalid TVFS folder size')
                        walk(table[at:at + length], name, level + 1)
                        at += length
                    else:
                        leaf(name, node)
                    name = parent
                elif at < len(table):
                    name += '/'
        if len(path) > 5 and path[0] == 255:
            if not be(path[1:5]) & 0x80000000:
                raise ValueError('Invalid TVFS directory root')
            path = path[5:]
        walk(path, prefix, 0)
