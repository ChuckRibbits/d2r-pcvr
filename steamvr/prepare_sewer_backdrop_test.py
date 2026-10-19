"""Synthetic fixtures only; no game geometry, presets or saves."""
import copy
import json
import math
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import prepare_sewer_backdrop as preview
from prepare_dev_scenarios import MOD_INFO


def measurements():
    half = math.sqrt(.5)
    return dict(center=(1., -2., -5.), facing=(half, 0., 0., half),
                scale=(.08, 1., .06), quad_center=(2., 0., .2),
                source_sha256={preview.STAIRS: 'example', preview.BLACK: 'example'})


def preset():
    return dict(type='Preset', name='synthetic', terrain={'preserved': True},
                dependencies={'models': [{'path': preview.STAIRS+'.model'}]},
                entities=[dict(type='Entity', name='stairs', id=42, components=[
                    dict(type='TransformDefinitionComponent', position=dict(x=7, y=8, z=9),
                         orientation=dict(x=0, y=0, z=0, w=1), scale=dict(x=1, y=1, z=1)),
                    dict(type='ModelDefinitionComponent', filename=preview.STAIRS+'.model'),
                    dict(type='PhysicsBodyDefinitionComponent', fixturedefs=['preserved'])])])


class BackdropGeometryTests(unittest.TestCase):
    def test_original_entities_and_physics_preserved(self):
        source = preset()
        original = copy.deepcopy(source)
        result = preview.backdrop_entities(source, measurements())
        self.assertEqual(source, original)
        self.assertEqual(result['entities'][:-2], original['entities'])
        self.assertEqual(result['terrain'], original['terrain'])
        self.assertEqual(result['dependencies']['models'],
                         original['dependencies']['models']+[{'path': preview.BLACK+'.model'}])
        for entity in result['entities'][-2:]:
            self.assertEqual([c['type'] for c in entity['components']],
                             ['TransformDefinitionComponent', 'ModelDefinitionComponent'])
            model = entity['components'][1]
            self.assertEqual(model['filename'], preview.BLACK+'.model')
            self.assertEqual((model['shadowMask'], model['lightMask']), (0, 0))
            self.assertFalse(model['floorModel'])

    def test_both_faces_share_center_and_follow_stair_rotation_and_scale(self):
        source, measured = preset(), measurements()
        half = math.sqrt(.5)
        parent = source['entities'][0]['components'][0]
        parent['orientation'] = dict(x=0, y=half, z=0, w=half)
        parent['scale'] = dict(x=2, y=2, z=2)
        result = preview.backdrop_entities(source, measured)
        # (1,-2,-5), rotated 90 degrees about Y and scaled 2, plus (7,8,9).
        expected_center = (-3., 4., 7.)
        normals = []
        for entity in result['entities'][-2:]:
            transform = entity['components'][0]
            position = preview.vector(transform['position'])
            orientation = preview.vector(transform['orientation'], 'xyzw')
            scale = preview.vector(transform['scale'])
            center = preview.add(position, preview.rotate(
                tuple(scale[i]*measured['quad_center'][i] for i in range(3)), orientation))
            for actual, expected in zip(center, expected_center):
                self.assertAlmostEqual(actual, expected)
            normals.append(preview.rotate((0., 1., 0.), orientation))
        self.assertAlmostEqual(sum(a*b for a, b in zip(*normals)), -1.)

    def test_missing_or_multiple_stairwells_refused(self):
        for count in (0, 2):
            source = preset()
            source['entities'] *= count
            with self.assertRaises(ValueError):
                preview.backdrop_entities(source, measurements())

    def test_invalid_transforms_refused(self):
        for key, value in (('position', dict(x=float('nan'), y=0, z=0)),
                           ('orientation', dict(x=0, y=0, z=0, w=2)),
                           ('scale', dict(x=1, y=2, z=1)),
                           ('scale', dict(x=-1, y=-1, z=-1))):
            source = preset()
            source['entities'][0]['components'][0][key] = value
            with self.assertRaises(ValueError):
                preview.backdrop_entities(source, measurements())

    def test_repeated_backdrop_and_id_conflict_refused(self):
        generated = preview.backdrop_entities(preset(), measurements())
        with self.assertRaises(ValueError):
            preview.backdrop_entities(generated, measurements())
        source = preset()
        source['entities'][0]['id'] = generated['entities'][-2]['id']
        with self.assertRaises(ValueError):
            preview.backdrop_entities(source, measurements())

    def test_only_four_entrance_presets_generated(self):
        class Assets:
            requested = []

            def read(self, name):
                self.requested.append(name)
                return json.dumps(preset()).encode()

        assets = Assets()
        with patch.object(preview, 'geometry', return_value=measurements()):
            payload, measured = preview.generate(assets)
        self.assertEqual(tuple(assets.requested), preview.PRESETS)
        self.assertEqual(set(payload), set(preview.PRESETS))
        self.assertEqual(measured, measurements())
        self.assertTrue(all(len(json.loads(data)['entities']) == 3 for data in payload.values()))


class BackdropLifecycleTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.game = Path(self.temp.name).resolve()
        self.root = self.game/'mods/pcvr-dev/pcvr-dev.mpq'
        self.root.mkdir(parents=True)
        (self.root/'modinfo.json').write_text(MOD_INFO, encoding='utf-8')
        self.payload = {name: b'{"synthetic":true}\n' for name in preview.PRESETS}

    def test_round_trip_keeps_other_files(self):
        keep = self.root/'unrelated.txt'
        keep.write_bytes(b'keep')
        preview.install_preview(preview.checked_root(self.game), self.payload, measurements())
        for name, expected in self.payload.items():
            self.assertEqual((self.root/name).read_bytes(), expected)
        self.assertEqual(preview.remove_preview(self.root), 4)
        self.assertEqual(keep.read_bytes(), b'keep')
        self.assertEqual(preview.checked_root(self.game), self.root)
        self.assertEqual(preview.remove_preview(self.root), 0)

    def test_existing_override_refuses_all_writes(self):
        existing = self.root/preview.PRESETS[-1]
        existing.parent.mkdir(parents=True)
        existing.write_bytes(b'unrelated')
        with self.assertRaises(ValueError):
            preview.install_preview(self.root, self.payload, measurements())
        self.assertEqual(existing.read_bytes(), b'unrelated')
        self.assertFalse((self.root/preview.PRESETS[0]).exists())
        self.assertFalse((self.root/preview.RECEIPT).exists())

    def test_failed_write_rolls_back_completed_overrides(self):
        original = Path.rename
        calls = []

        def rename(path, target):
            calls.append(target)
            if len(calls) == 2:
                raise OSError('simulated copy failure')
            return original(path, target)

        with patch.object(Path, 'rename', rename):
            with self.assertRaisesRegex(OSError, 'simulated'):
                preview.install_preview(self.root, self.payload, measurements())
        self.assertEqual(len(calls), 2)
        self.assertFalse(any((self.root/name).exists() for name in (*preview.PRESETS, preview.RECEIPT)))
        self.assertFalse(list(self.root.rglob('.pcvr-sewer-*')))

    def test_modified_override_refuses_all_removals(self):
        preview.install_preview(self.root, self.payload, measurements())
        (self.root/preview.PRESETS[-1]).write_bytes(b'modified')
        with self.assertRaises(ValueError):
            preview.remove_preview(self.root)
        self.assertTrue(all((self.root/name).exists() for name in (*preview.PRESETS, preview.RECEIPT)))

    def test_partial_install_can_be_removed(self):
        preview.install_preview(self.root, self.payload, measurements())
        (self.root/preview.PRESETS[-1]).unlink()
        self.assertEqual(preview.remove_preview(self.root), 3)

    def test_paths_and_receipt_cannot_target_other_files(self):
        for name in ('../modinfo.json', 'data/hd/env/preset/act1/house.json', str(self.game/'outside.json')):
            with self.assertRaises(ValueError):
                preview.checked_path(self.root, name)
            files = {key: preview.digest(value) for key, value in self.payload.items()}
            files[name] = preview.digest(b'')
            (self.root/preview.RECEIPT).write_text(json.dumps(dict(format=1, files=files)))
            with self.assertRaises(ValueError):
                preview.remove_preview(self.root)
        self.assertEqual(preview.checked_root(self.game), self.root)

    def test_normal_save_path_refused(self):
        (self.root/'modinfo.json').write_text('{"name":"pcvr-dev","savepath":"../"}')
        with self.assertRaises(ValueError):
            preview.checked_root(self.game)


class InstallerBackdropTests(unittest.TestCase):
    def payload(self, mod='D2R-PCVR', data=b'synthetic preset'):
        payload = {name: data for name in preview.install_names(mod)}
        payload[preview.INSTALL_RECEIPT] = json.dumps(dict(version=1, sha256={
            name: preview.digest(value) for name, value in payload.items()})).encode()
        return payload

    def test_preparation_maps_both_mods_without_writing_or_changing_geometry(self):
        class Assets:
            build_hash = 'synthetic build'

            def __enter__(self): return self
            def __exit__(self, *args): pass

        original = {name: b'synthetic local preset' for name in preview.PRESETS}
        for mod in ('D2R-PCVR', 'pcvr-dev'):
            with self.subTest(mod=mod), patch.object(preview, 'open_assets', return_value=Assets()) as opened, \
                    patch.object(preview, 'generate', return_value=(original, measurements())):
                payload = preview.prepare(Path('synthetic game'), mod, casclib=Path('synthetic CascLib'))
            opened.assert_called_once_with(Path('synthetic game'), Path('synthetic CascLib'))
            self.assertEqual(set(payload), set(preview.install_names(mod)) | {preview.INSTALL_RECEIPT})
            for name in preview.PRESETS:
                self.assertEqual(payload[f'{mod}.mpq/{name}'], original[name])
            record = json.loads(payload[preview.INSTALL_RECEIPT])
            self.assertEqual(record['build'], Assets.build_hash)
            self.assertEqual(record['geometry']['center'], list(measurements()['center']))
            self.assertEqual(record['sha256'], {name: preview.digest(payload[name]) for name in preview.install_names(mod)})

    def test_owned_updates_and_identical_repairs_are_read_only(self):
        with tempfile.TemporaryDirectory() as tmp:
            mod = Path(tmp).resolve()
            previous, updated = self.payload(), self.payload(data=b'updated synthetic preset')
            preview.check_existing(mod, previous)
            for name, data in previous.items():
                path = mod/name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
            unrelated = mod/'unrelated.txt'
            unrelated.write_bytes(b'keep')
            before = {p: p.read_bytes() for p in mod.rglob('*') if p.is_file()}
            preview.check_existing(mod, previous)
            preview.check_existing(mod, updated)
            self.assertEqual(before, {p: p.read_bytes() for p in mod.rglob('*') if p.is_file()})

    def test_custom_and_untracked_presets_refuse_the_entire_payload(self):
        for tracked in (False, True):
            with self.subTest(tracked=tracked), tempfile.TemporaryDirectory() as tmp:
                mod = Path(tmp).resolve()
                payload = self.payload()
                path = mod/preview.install_names('D2R-PCVR')[-1]
                path.parent.mkdir(parents=True)
                path.write_bytes(b'player edit')
                if tracked:
                    (mod/preview.INSTALL_RECEIPT).write_bytes(payload[preview.INSTALL_RECEIPT])
                before = {p: p.read_bytes() for p in mod.rglob('*') if p.is_file()}
                with self.assertRaisesRegex(ValueError, 'left unchanged'):
                    preview.check_existing(mod, payload)
                self.assertEqual(before, {p: p.read_bytes() for p in mod.rglob('*') if p.is_file()})

    def test_unknown_mod_and_incomplete_payloads_refused(self):
        with self.assertRaises(ValueError):
            preview.prepare(None, '../D2R-VR')
        with tempfile.TemporaryDirectory() as tmp:
            mod = Path(tmp).resolve()
            payload = self.payload()
            for altered in ({}, {**payload, '../elsewhere': b'bad'}, self.payload('pcvr-dev')):
                with self.assertRaises(ValueError):
                    preview.check_existing(mod, altered)
            self.assertFalse(list(mod.rglob('*')))

    def test_invalid_ownership_receipt_refuses_updates(self):
        records = [[], dict(version=2, sha256={}), dict(version=1, sha256={'../elsewhere': 'bad'})]
        with tempfile.TemporaryDirectory() as tmp:
            mod = Path(tmp).resolve()
            for record in records:
                (mod/preview.INSTALL_RECEIPT).write_text(json.dumps(record))
                with self.assertRaisesRegex(ValueError, 'Unknown sewer'):
                    preview.check_existing(mod, self.payload())


if __name__ == '__main__':
    unittest.main()
