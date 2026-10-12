"""Read assets locally using the installation's archive format."""
import hashlib
from pathlib import Path
from game_install import storage_config
from steam_assets import SteamAssets


def build_hash(game):
    _, config = storage_config(game)
    return hashlib.sha256(config.read_bytes()).hexdigest()


def open_assets(game, casclib=None):
    store, _ = storage_config(game)
    if store == 'steam':
        return SteamAssets(game)
    from casc_assets import CascAssets
    return CascAssets(game, casclib or Path(__file__).resolve().parents[1] / 'bin/CascLib.dll')
