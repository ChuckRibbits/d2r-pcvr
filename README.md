# D2R PCVR

Play **Diablo II: Resurrected in VR on Windows**, with support for
**SteamVR and Virtual Desktop (VDXR)**. Use motion controls to swing your
weapon, aim spells, drink belt potions and explore Sanctuary in first person.

**[Download the Windows preview](https://github.com/bearstonem/d2r-pcvr/releases/tag/v0.3.0-windows-quest-preview)**
and follow **[Install and play](INSTALL-WINDOWS.md)**.
Download **d2r-pcvr-windows.zip** from the release's Assets section.

## What you need

- A Windows PC with the **Steam edition of D2R 3.3.93787**.
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
- Free-swing melee, aimed spells and bow controls.
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

See [Install and play](INSTALL-WINDOWS.md) for setup, controls and removal.

Based on [BodyWalkVR's D2R VR](https://github.com/natpoh/diablo2-vr-port).
Licenses and credits are in [THIRD_PARTY.md](THIRD_PARTY.md) and [LICENSE](LICENSE).
This project is not affiliated with or endorsed by Blizzard Entertainment.
