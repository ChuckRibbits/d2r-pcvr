"""Cache contracts and invalid-input tests; no Blizzard data in fixtures."""
import math
import struct
import unittest
from prepare_potions import encode
from potion_assets import Granny,texture,decode

class PotionTests(unittest.TestCase):
    def test_normalization_top_axis_and_topology(self):
        # Offset source model whose neck is not at its native origin.
        verts=[(1,0,3,0,1,0,0,0),(2,0,3,0,1,0,1,0),(1.5,4,3,0,1,0,.5,1)]
        result=encode([('glassShape',verts,[0,1,2])],(4,4,78,[bytes(16)]))
        h=struct.unpack_from('<8I',result)
        self.assertEqual(h,(0x31525650,1,3,3,4,4,78,1))
        v=list(struct.iter_unpack('<9f',result[32:32+3*36]))
        self.assertAlmostEqual(v[2][0],0);self.assertAlmostEqual(v[2][1],.09);self.assertAlmostEqual(v[2][2],0)
        self.assertAlmostEqual(v[0][1],-.09);self.assertEqual(v[0][-1],1)
        self.assertEqual(struct.unpack_from('<3I',result,32+3*36),(0,1,2))

    def test_texture_relative_offset_mips_and_truncation(self):
        data=bytearray(68)
        struct.pack_into('<IHHII',data,0,0x2845443c,62,0,4,4)
        struct.pack_into('<I',data,28,1)
        struct.pack_into('<II',data,36,16,12)
        data[52:68]=bytes(range(16))
        self.assertEqual(texture(data),(4,4,78,[bytes(range(16))]))
        with self.assertRaises(ValueError):texture(data[:-1])
        struct.pack_into('<I',data,40,0)
        with self.assertRaises(ValueError):texture(data)

    def test_parser_bounds_before_decompression(self):
        for data in (b'',bytes(104),bytes.fromhex('e59b495e6f631f141e13eba990beedc4')+bytes(100)):
            with self.assertRaises(ValueError):Granny(data)
        with self.assertRaises(ValueError):decode(b'',100*1024*1024,4)
        with self.assertRaises(ValueError):decode(b'',4,1)
        with self.assertRaises(ValueError):decode(b'abc',4,0)

if __name__=='__main__':unittest.main()
