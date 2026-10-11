"""Portable cache tests with synthetic data; no Blizzard assets or game needed."""
import hashlib
import io
from pathlib import Path
import struct
import tempfile
import unittest
import zlib
from prepare_icons import sprite_frame, png, item_key
from steam_assets import SteamAssets, canonical, inflate, exact


def sprite(width, height, frame_width, frames, pixels):
    header = bytearray(40)
    header[:4] = b'SpA1'
    struct.pack_into('<HHII', header, 4, 31, frame_width, width, height)
    struct.pack_into('<I', header, 20, frames)
    struct.pack_into('<II', header, 32, len(pixels), 4)
    return bytes(header) + pixels


class NativeIconTests(unittest.TestCase):
    def test_sheet_frame_gutters_and_row_pitch(self):
        # Red, gutter, blue, gutter. Frame reads must not bleed into neighbors.
        red, blue, gap = bytes([255, 0, 0, 255]), bytes([0, 0, 255, 100]), bytes(4)
        for final_gutter in (True, False):
            row = red * 2 + gap + blue * 2 + (gap if final_gutter else b'')
            data = sprite(len(row) // 4, 3, 2, 2, row * 3)
            self.assertEqual(sprite_frame(data, 1), (2, 3, blue * 6))
            self.assertEqual(sprite_frame(data, 0), (2, 3, red * 6))
            with self.assertRaises(ValueError): sprite_frame(data, 2)
            with self.assertRaises(ValueError): sprite_frame(data[:-1])

    def test_invalid_header_and_dimension_rejection(self):
        data = sprite(2, 1, 2, 1, bytes(8))
        for at, value in ((0, 0), (4, 30), (6, 8), (20, 0), (36, 3)):
            corrupt = bytearray(data); corrupt[at] = value
            with self.subTest(at=at), self.assertRaises(ValueError): sprite_frame(corrupt)

    def test_png_keeps_alpha_and_chunk_checksums(self):
        rgba = bytes([4, 20, 99, 128, 200, 0, 0, 0])
        image = png(2, 1, rgba)
        self.assertEqual(image[:8], b'\x89PNG\r\n\x1a\n')
        pos = 8; chunks = {}
        while pos < len(image):
            n = int.from_bytes(image[pos:pos+4], 'big')
            kind, data = image[pos+4:pos+8], image[pos+8:pos+8+n]
            self.assertEqual(int.from_bytes(image[pos+8+n:pos+12+n], 'big'), zlib.crc32(kind+data))
            chunks[kind] = data; pos += 12+n
        self.assertEqual(zlib.decompress(chunks[b'IDAT']), b'\0'+rgba)
        self.assertEqual(item_key('hax'), 'i20786168')

    def test_read_only_blte_checksum_and_bounded_inflation(self):
        with tempfile.TemporaryDirectory() as temp:
            asset = b'local synthetic icon bytes' * 3
            encoded = b'Z' + zlib.compress(asset)
            table = struct.pack('>II', len(encoded), len(asset)) + hashlib.md5(encoded).digest()
            blob = b'BLTE' + struct.pack('>I', 36) + b'\x0f\0\0\x01' + table + encoded
            path = Path(temp) / '00-00000000.data'; path.write_bytes(blob)
            reader = SteamAssets.__new__(SteamAssets)
            reader.handles = {}; reader.directory = Path(temp); reader.index_bits = 0
            reader.layouts = {0: (0, 0, 56, 1)}
            try:
                self.assertEqual(reader.blob(bytes(16)), asset)
                reader.files = {'data/test': [(0, len(asset), bytes(16))]}
                reader.digests = {'data/test': '0' * 32}
                with self.assertRaisesRegex(ValueError, 'checksum'): reader.read('data/test')
                reader.close()
                path.write_bytes(blob[:-1] + bytes([blob[-1] ^ 1]))
                with self.assertRaisesRegex(ValueError, 'checksum'): reader.blob(bytes(16))
            finally: reader.close()
            self.assertEqual(list(Path(temp).iterdir()), [path])
        with self.assertRaises(ValueError): inflate(zlib.compress(bytes(300)), 20)
        with self.assertRaises(ValueError): exact(io.BytesIO(b'x'), 2)
        with self.assertRaises(ValueError): canonical('data/../save')
        self.assertEqual(canonical('data:data\\hd\\test.sprite'), 'data/hd/test.sprite')


if __name__ == '__main__': unittest.main()
