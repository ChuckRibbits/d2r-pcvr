"""Identify local game storage and gate builds before installing any files."""
from pathlib import Path

STEAM_VERSION = '3.3.93787'
BATTLENET_VERSION = '3.3.93847'


def storage_config(game):
    game = Path(game)
    for directory in ('Data', 'data'):
        config = game / directory / '.build.config'
        if config.is_file():
            return 'steam', config
    if (game / '.build.info').is_file():
        return 'battlenet', game / '.build.info'
    raise ValueError('Game archive configuration not found (.build.config or .build.info).')


def validate_game(game, store='auto', experimental_battlenet=False):
    game = Path(game)
    detected, _ = storage_config(game)
    if store != 'auto' and detected != store:
        raise ValueError(f'This is a {detected} installation, but {store} was selected.')
    expected = STEAM_VERSION if detected == 'steam' else BATTLENET_VERSION
    exe = game / 'D2R.exe'
    if not exe.is_file() or expected.encode('utf-16-le') not in exe.read_bytes():
        raise ValueError(f'This build requires {detected} D2R {expected}. Your game version does not match.')
    if detected == 'battlenet' and not experimental_battlenet:
        raise ValueError('Battle.net support is experimental. Use --experimental-battlenet for local offline compatibility testing.')
    return detected, expected


def battlenet_location():
    """Read the game's uninstall registration, never Battle.net account data."""
    import winreg
    for hive in (winreg.HKEY_LOCAL_MACHINE, winreg.HKEY_CURRENT_USER):
        for base in (r'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall',
                     r'SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall'):
            try:
                with winreg.OpenKey(hive, base + r'\Diablo II Resurrected') as handle:
                    path = Path(winreg.QueryValueEx(handle, 'InstallLocation')[0])
                if (path / 'D2R.exe').is_file():
                    return path.resolve()
            except OSError:
                pass
    return None
