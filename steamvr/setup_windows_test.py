"""Setup safety and routing fixtures. Never read/write the user's Steam profile."""
import contextlib
import io
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import setup_windows as setup
import steam_config as config


LOCAL = '"UserLocalConfigStore"\r\n{\r\n// untouched comment\r\n"friends" { "PersonaName" "Player" }\r\n"Software" { "Valve" { "Steam" { "apps" { "2536520" { "LaunchOptions" "old option" "Other" "keep" } "123" { "LaunchOptions" "unrelated" } } } } }\r\n"PrivateData" "fixture only"\r\n}\r\n'


class SteamSetupTests(unittest.TestCase):
    def test_release_integrity_rejects_changed_file_and_escaping_path(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            names = ('bin/d2rl-vrcam.dll', 'bin/openxr_loader.dll', 'steamvr/install.py',
                     'steamvr/setup_windows.py', 'steamvr/steam_config.py', 'steamvr/launch_windows.py',
                     'steamvr/game_install.py', 'steamvr/game_assets.py', 'steamvr/casc_assets.py',
                     'Uninstall-Windows.cmd', 'steamvr/uninstall_windows.py')
            hashes = {}
            for name in names:
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'fixture')
                hashes[name] = hashlib.sha256(b'fixture').hexdigest()
            manifest = {'platform': 'native-windows', 'sha256': hashes}
            source = root / 'release-manifest.json'
            source.write_text(json.dumps(manifest))
            self.assertEqual(setup.verify_release(root), manifest)
            (root / names[0]).write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError, 'missing or changed'):
                setup.verify_release(root)
            (root / names[0]).write_bytes(b'fixture')
            hashes['../outside'] = hashes[names[0]]
            source.write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, 'Invalid path'):
                setup.verify_release(root)

    def test_launch_options_change_only_target_span(self):
        options = setup.launch_options(Path('C:/Mods with spaces/python.exe'), Path('C:/Mods with spaces/launch_windows.py'), Path('D:/Steam VR/steamxr_win64.json'))
        updated = config.set_launch_options(LOCAL, options)
        self.assertEqual(updated, LOCAL.replace('"old option"', config.quote(options)))
        self.assertEqual(config.text_value(config.parse_text(updated), 'UserLocalConfigStore', 'Software', 'Valve', 'Steam', 'apps', '2536520', 'LaunchOptions'), options)
        self.assertEqual(config.set_launch_options(updated, options), updated)
        self.assertEqual(options.count('%command%'), 1)
        self.assertNotIn('--frame-vr', options)

    def test_missing_app_and_ancestors_and_bom(self):
        for local in ('"UserLocalConfigStore" {}', '"UserLocalConfigStore" { "Software" { "Valve" { "Steam" { "apps" {} } } } }', '\ufeff' + LOCAL):
            updated = config.set_launch_options(local, '"C:\\mod\\python.exe" %command%')
            value = config.text_value(config.parse_text(updated.lstrip('\ufeff')), 'UserLocalConfigStore', 'Software', 'Valve', 'Steam', 'apps', '2536520', 'LaunchOptions')
            self.assertEqual(value, '"C:\\mod\\python.exe" %command%')
            self.assertEqual(updated.startswith('\ufeff'), local.startswith('\ufeff'))

    def test_ambiguous_or_malformed_configuration_rejected(self):
        for local in ('"UserLocalConfigStore" { "Software" {} "software" {} }', '"UserLocalConfigStore" {', '"OtherRoot" {}'):
            with self.assertRaises(ValueError):
                config.set_launch_options(local, 'new')
        for data in (b'\x00shortcuts\0', b'\x07bad\0\x08', b'\x08trailing'):
            with self.assertRaises(ValueError):
                config.update_shortcuts(data, Path('steam'), Path('game'), ['steamvr'])

    def test_shortcuts_idempotent_preserve_other_entries_and_select_runtime(self):
        other = {'appid': 0xfedcba98, 'appname': 'Other Game', 'Exe': '"other.exe"', 'tags': {'0': 'favorites'}, 'UnknownField': 'retained'}
        original = config.encode_binary({'shortcuts': {'0': other}})
        updated = config.update_shortcuts(original, Path('C:/Steam'), Path('D:/D2R'), ['steamvr', 'vdxr'])
        self.assertEqual(config.update_shortcuts(updated, Path('C:/Steam'), Path('D:/D2R'), ['steamvr', 'vdxr']), updated)
        entries = config.decode_binary(updated)['shortcuts']
        self.assertEqual(entries['0'], other)
        self.assertEqual(entries['1']['OpenVR'], 1)
        self.assertEqual(entries['2']['OpenVR'], 0)
        self.assertEqual(entries['1']['LaunchOptions'], '-applaunch 2536520 --frame-vr')
        self.assertEqual(entries['2']['LaunchOptions'], '-applaunch 2536520 --frame-vr --frame-vdxr')
        only_vd = config.decode_binary(config.update_shortcuts(None, Path('Steam'), Path('Game'), ['vdxr']))['shortcuts']
        self.assertEqual(len(only_vd), 1)
        self.assertEqual(only_vd['0']['OpenVR'], 0)

    def fixture(self, root):
        profile = root / 'userdata/123'
        directory = profile / 'config'
        directory.mkdir(parents=True)
        (directory / 'localconfig.vdf').write_bytes(LOCAL.encode())
        return profile, directory

    def test_transaction_backup_and_absent_shortcuts(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            profile, directory = self.fixture(root)
            backup = config.configure(profile, root, root, 'new', ['vdxr'], root / 'backups', lambda: None)
            self.assertEqual((backup / 'localconfig.vdf').read_bytes(), LOCAL.encode())
            self.assertFalse((backup / 'shortcuts.vdf').exists())
            self.assertEqual((directory / 'localconfig.vdf').read_bytes(), LOCAL.replace('"old option"', '"new"').encode())
            self.assertEqual(config.decode_binary((directory / 'shortcuts.vdf').read_bytes())['shortcuts']['0']['OpenVR'], 0)
            self.assertFalse(list(directory.glob('d2r-setup-*')))

    def test_second_file_write_failure_rolls_back_first(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            profile, directory = self.fixture(root)
            old_shortcuts = config.encode_binary({'shortcuts': {}})
            (directory / 'shortcuts.vdf').write_bytes(old_shortcuts)
            replace = config.os.replace
            calls = 0
            def fail_second(source, target):
                nonlocal calls
                calls += 1
                if calls == 2:
                    raise PermissionError('fixture blocked write')
                return replace(source, target)
            with patch.object(config.os, 'replace', side_effect=fail_second), self.assertRaises(PermissionError):
                config.configure(profile, root, root, 'new', ['steamvr'], root / 'backups', lambda: None)
            self.assertEqual((directory / 'localconfig.vdf').read_bytes(), LOCAL.encode())
            self.assertEqual((directory / 'shortcuts.vdf').read_bytes(), old_shortcuts)
            self.assertFalse(list(directory.glob('d2r-setup-*')))

    def test_steam_reopening_or_external_change_prevents_commit(self):
        for race in ('steam', 'config'):
            with self.subTest(race=race), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                profile, directory = self.fixture(root)
                calls = 0
                def check():
                    nonlocal calls
                    calls += 1
                    if calls == 2:
                        if race == 'steam':
                            raise ValueError('Steam reopened')
                        (directory / 'localconfig.vdf').write_text('changed externally')
                with self.assertRaises(ValueError):
                    config.configure(profile, root, root, 'new', ['steamvr'], root / 'backups', check)
                expected = LOCAL.encode() if race == 'steam' else b'changed externally'
                self.assertEqual((directory / 'localconfig.vdf').read_bytes(), expected)
                self.assertFalse((directory / 'shortcuts.vdf').exists())

    def test_discovery_uses_separate_libraries_and_limits_manifest_path(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            steam = root / 'Steam'
            library = root / 'Second library'
            (steam / 'steamapps').mkdir(parents=True)
            (library / 'steamapps/common/D2R').mkdir(parents=True)
            (steam / 'steamapps/libraryfolders.vdf').write_text('"libraryfolders" { "0" { "path" ' + config.quote(str(steam)) + ' } "1" { "path" ' + config.quote(str(library)) + ' } }')
            manifest = library / 'steamapps/appmanifest_2536520.acf'
            manifest.write_text('"AppState" { "installdir" "D2R" }')
            roots = setup.libraries(steam)
            self.assertEqual(roots, [steam, library])
            self.assertEqual(setup.app_folder(roots, '2536520'), library / 'steamapps/common/D2R')
            manifest.write_text('"AppState" { "installdir" "../../../escape" }')
            self.assertIsNone(setup.app_folder(roots, '2536520'))

    def test_wizard_quest_vdxr_shortcuts_only_and_frame_install_routing(self):
        for quest, battlenet_in_package in ((True, False), (False, False), (True, True), (False, True)):
            with self.subTest(quest=quest, battlenet_in_package=battlenet_in_package), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                steam = root / 'steam'
                game = root / 'game'
                saves = root / 'saves'
                steam.mkdir()
                game.mkdir()
                saves.mkdir()
                profile, _ = self.fixture(steam)
                (game / 'D2R.exe').write_bytes('3.3.93787'.encode('utf-16-le'))
                (game / 'Data').mkdir()
                (game / 'Data/.build.config').write_text('fixture')
                for name in ('D2RLoader.exe', 'ReShade64.dll', 'openxr_loader.dll', 'd2rloader/plugins/d2rl-vrcam.dll', 'd2rloader/plugins/d2r_vr.ini'):
                    file = game / name
                    file.parent.mkdir(parents=True, exist_ok=True)
                    file.write_bytes(b'fixture')
                # Quest: headset 2, VD 2, shortcut-only 3, continue 1, open later 2.
                # Frame: headset 1, preserve upgrade 1, continue 1, open later 2.
                answers = ['2', '2', '3', '1', '2'] if quest else ['1', '1', '1', '2']
                if battlenet_in_package:
                    answers.insert(0, '1')  # Select Steam in a package that offers both editions.
                manifest = {'release_tag': 'fixture', 'sha256': {'bin/CascLib.dll': 'fixture'} if battlenet_in_package else {}}
                folders = [steam, game, saves] if quest else [steam, game, root, saves]
                with patch.object(setup, 'ROOT', root), patch.object(setup, 'verify_release', return_value=manifest), \
                        patch.object(setup, 'folder', side_effect=folders), patch.object(setup, 'steam_location', return_value=steam), \
                        patch.object(setup, 'saved_games', return_value=saves), patch.object(setup, 'wait_for_closed'), \
                        patch.object(setup, 'require_closed'), patch.object(setup.subprocess, 'run') as run, \
                        patch.object(setup.subprocess, 'Popen') as popen, patch('builtins.input', side_effect=answers), \
                        patch('launch_windows.vdxr_manifest', return_value=game / 'openxr_loader.dll'), \
                        patch.object(setup.sys, 'argv', ['setup_windows.py']), contextlib.redirect_stdout(io.StringIO()) as output:
                    setup.main()
                result = json.loads((root / 'setup-result.json').read_text())
                self.assertEqual(result['store'], 'steam')
                self.assertFalse(result['experimental'])
                state = setup.install.read_record(game)['steam_profiles'][str(profile.resolve())]
                self.assertEqual(state['previous_launch_options'], 'old option')
                self.assertEqual(state['installed_launch_options'], result['launch_options'])
                self.assertEqual('Which game installation?' in output.getvalue(), battlenet_in_package)
                self.assertEqual(result['connections'], ['vdxr'] if quest else ['steamvr'])
                entries = config.decode_binary((profile / 'config/shortcuts.vdf').read_bytes())['shortcuts']
                self.assertEqual(entries['0']['OpenVR'], 0 if quest else 1)
                if quest:
                    run.assert_not_called()
                    self.assertNotIn('--frame-openxr-runtime=', result['launch_options'])
                else:
                    self.assertIn('--preserve-settings', run.call_args.args[0])
                    self.assertNotIn('--experimental-battlenet', run.call_args.args[0])
                    self.assertIn('--frame-openxr-runtime=', result['launch_options'])
                popen.assert_not_called()


if __name__ == '__main__':
    unittest.main()
