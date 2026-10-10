#!/usr/bin/env python3
"""Installer reversibility and portable launch routing, using temporary fixtures."""
import contextlib
import configparser
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

import install
import launch_windows


class InstallationTests(unittest.TestCase):
    def test_install_restore_keeps_progress_and_stock_executable(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            game = root / 'library/steamapps/common/Diablo II Resurrected'
            game.mkdir(parents=True)
            exe = '3.3.93787'.encode('utf-16-le')
            (game / 'D2R.exe').write_bytes(exe)
            (game / 'ReShade.ini').write_text('existing config')
            icon_dir = game / 'd2rloader/plugins/d2r_vr_icons'
            icon_dir.mkdir(parents=True)
            (icon_dir / 'manifest.tsv').write_text('old local cache')
            potion_dir = game / 'd2rloader/plugins/d2r_vr_potions'
            potion_dir.mkdir()
            (potion_dir / 'hp1.pvr').write_bytes(b'previous potion')
            saves = install.default_saves(game)
            saves.mkdir(parents=True)
            (saves / 'Settings.json').write_text('{"Resolution Scale": 100}')
            (saves / 'Friend.d2s').write_bytes(b'original progress')
            deps = root / 'deps'
            deps.mkdir()
            archive = deps / 'D2RLoader-1.3.1-beta.zip'
            with zipfile.ZipFile(archive, 'w') as z:
                z.writestr('D2RLoader.exe', b'fixture loader')
                z.writestr('d2rloader/config/d2rloader.toml', 'fixture config')
            (deps / 'reshade-6.8').mkdir()
            (deps / 'reshade-6.8/ReShade64.dll').write_bytes(b'fixture reshade')
            compiler = deps / 'd3dcompiler47/modern/d3dcompiler_47.dll'
            compiler.parent.mkdir(parents=True)
            compiler.write_bytes(b'fixture compiler')
            plugin = root / 'plugin.dll'
            plugin.write_bytes(b'fixture plugin')
            loader = root / 'openxr_loader.dll'
            loader.write_bytes(b'fixture xr loader')
            common = ['install.py', '--game', str(game), '--deps', str(deps),
                      '--plugin', str(plugin), '--openxr-loader', str(loader),
                      '--saves', str(saves), '--backup-dir', str(root / 'backups')]
            def icons(game, output):
                out = Path(output)
                for name in ('fixture.png', 'manifest.tsv', 'cache.json'):
                    (out / name).write_bytes(b'local generated fixture')
                return {'images': ['fixture.png'], 'icons': 1}
            def potions(game, output):
                for name in [code + '.pvr' for code in install.POTION_CODES] + ['cache.json']:
                    (Path(output) / name).write_bytes(b'local potion fixture')
                return {'models': len(install.POTION_CODES)}
            with patch.object(install, 'require_game_closed'), contextlib.redirect_stdout(io.StringIO()), \
                    patch.object(install, 'COMPILER_HASH', hashlib.sha256(compiler.read_bytes()).hexdigest()), \
                    patch.object(install, 'prepare_icons', side_effect=icons), \
                    patch.object(install, 'prepare_potions', side_effect=potions):
                with patch.object(sys, 'argv', common), patch.object(install, 'LOADER_HASH', 'incorrect'):
                    with self.assertRaises(SystemExit):
                        install.main()
                self.assertFalse((root / 'backups').exists())
                digest = hashlib.sha256(archive.read_bytes()).hexdigest()
                with patch.object(sys, 'argv', common), patch.object(install, 'LOADER_HASH', digest):
                    install.main()
                snapshot = next((root / 'backups').iterdir())
                self.assertEqual((game / 'D2R.exe').read_bytes(), exe)
                self.assertFalse((game / 'dxgi.dll').exists())
                self.assertEqual((icon_dir / 'fixture.png').read_bytes(), b'local generated fixture')
                self.assertEqual((potion_dir / 'hp1.pvr').read_bytes(), b'local potion fixture')
                self.assertTrue((potion_dir / 'rvl.pvr').is_file())
                self.assertEqual((game / 'd2rloader/runtime/d3dcompiler_47.dll').read_bytes(), compiler.read_bytes())
                self.assertFalse((game / 'd3dcompiler_47.dll').exists())
                cfg=configparser.ConfigParser(allow_no_value=True)
                cfg.read(game / 'd2rloader/plugins/d2r_vr.ini')
                self.assertTrue(cfg.getboolean('render','symmetric_projection'))
                self.assertTrue(cfg.getboolean('hud','monster_draw_order'))
                self.assertFalse(cfg.getboolean('render','defer_gpu_wait'))
                self.assertEqual(json.loads((saves / 'Settings.json').read_text())['NVIDIA DLSS'],0)
                self.assertEqual(json.loads((saves / 'Settings.json').read_text())['Resolution Scale'], 80)
                (saves / 'Friend.d2s').write_bytes(b'new progress')
                with patch.object(sys, 'argv', ['install.py', '--game', str(game), '--restore', str(snapshot)]):
                    install.main()
                self.assertEqual((saves / 'Friend.d2s').read_bytes(), b'new progress')
                self.assertEqual((game / 'ReShade.ini').read_text(), 'existing config')
                self.assertEqual((game / 'D2R.exe').read_bytes(), exe)
                self.assertFalse((game / 'D2RLoader.exe').exists())
                self.assertFalse((icon_dir / 'fixture.png').exists())
                self.assertEqual((icon_dir / 'manifest.tsv').read_text(), 'old local cache')
                self.assertEqual((potion_dir / 'hp1.pvr').read_bytes(), b'previous potion')
                self.assertFalse((potion_dir / 'rvl.pvr').exists())
                self.assertFalse((game / 'openxr_loader.dll').exists())
                self.assertFalse((game / 'd2rloader/runtime/d3dcompiler_47.dll').exists())
                self.assertEqual(json.loads((saves / 'Settings.json').read_text())['Resolution Scale'], 100)

    def test_archive_paths_reject_windows_and_linux_traversal(self):
        for path in ('../D2R.exe', '/tmp/file', 'C:\\file', 'd2rloader\\..\\D2R.exe'):
            with self.subTest(path=path), self.assertRaises(SystemExit):
                install.safe_relative(path)

    def test_windows_steam_arguments_and_paths_with_spaces(self):
        original = ['C:\\Steam Library\\D2R.exe', '-some-argument', 'a value']
        self.assertEqual(launch_windows.prepare(original), (original, None))
        with patch.object(Path, 'is_file', return_value=True):
            command, cwd = launch_windows.prepare(original + ['--frame-vr'])
        self.assertEqual(command, ['C:\\Steam Library\\D2RLoader.exe', '-some-argument', 'a value'])
        self.assertEqual(cwd, 'C:\\Steam Library')


if __name__ == '__main__':
    unittest.main()
