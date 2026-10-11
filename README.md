# D2R PCVR

Play **Diablo II: Resurrected in VR on Windows**, with support for
**SteamVR and Virtual Desktop (VDXR)**. Use motion controls to swing your
weapon, aim spells, drink belt potions and explore Sanctuary in first person.

**This is a mod for the original game, not a standalone game. You must purchase
and install your own legitimate copy of Diablo II: Resurrected.** This Windows
preview requires the Steam edition. The mod adds VR support and motion
interactions on top of your existing game installation.

**No Blizzard game assets are included in this repository or the release ZIP.**
The download contains the mod and licensed third-party components, not the
game, its executable, models, textures or audio. During installation, required
item/skill icons and potion models are read from your own installed game into
local caches. Keep those caches local; do not include them when sharing the mod.

**[Download the Windows preview](https://github.com/bearstonem/d2r-pcvr/releases/tag/v0.3.1-windows-preview)**
and follow **[Install and play](INSTALL-WINDOWS.md)**.
Download **d2r-pcvr-windows.zip** from the release's Assets section.

## What you need

- A Windows PC with your **purchased Steam copy of D2R 3.3.93787** installed.
- A VR headset and two tracked controllers.
- **SteamVR**, or **Virtual Desktop for Quest with its Windows Streamer**.
- **Python 3.10 or newer** to run the included installer and launcher.
- A **local offline character**. This preview is for offline single-player.

## Headset connections

| Headset | Connection |
| --- | --- |
| Steam Frame | SteamVR streaming |
| Quest 3 | Steam Link with SteamVR, or Virtual Desktop with VDXR |
| Quest 2 | The same Quest connection options; compatibility is not yet confirmed |

This is a PCVR mod: the game runs on your Windows PC and streams to the headset.

## In the game

- First-person VR with your character's body, hands and equipment.
- Free-swing melee, including with a wheel skill selected, aimed spells and bow controls.
- Belt potions you can grab and drink, plus wrist health and mana indicators.
- Body calibration for your height and arm span, available on first load
  and through **Left Grip + Y → Calibrate body**.
- Weapon-set swapping: reach your right hand over either shoulder and press
  its trigger. You can change the swap hand in VR settings.
- Inventory, skills, conversations and Journal through the game menu:
  **Frame Left A / Quest Left X**.

After setup, connect your headset and double-click **Launch-VR-SteamVR.cmd**
or **Launch-VR-VDXR.cmd**. Select an offline character when the game opens.
For ordinary desktop play, use Steam's normal **Play** button. Fully exit
the VR session before switching to desktop play or another headset connection.
You can also add separate **SteamVR** and **Virtual Desktop** entries to your
Steam library using the [shortcut instructions](INSTALL-WINDOWS.md#add-steam-library-shortcuts).

See [Install and play](INSTALL-WINDOWS.md) for setup, controls and removal.

Based on [BodyWalkVR's D2R VR](https://github.com/natpoh/diablo2-vr-port).
Licenses and credits are in [THIRD_PARTY.md](THIRD_PARTY.md) and [LICENSE](LICENSE).
This project is not affiliated with or endorsed by Blizzard Entertainment.
