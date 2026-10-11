# D2R PCVR v0.3.4 — Windows preview

Play **Diablo II: Resurrected in VR on Windows** with
**SteamVR or Virtual Desktop (VDXR)**. This preview is for **local offline
single-player** and requires the **Steam edition of D2R 3.3.93787**.

**This download is a mod, not the original game. You must purchase and install
your own legitimate Steam copy of Diablo II: Resurrected.** D2R PCVR adds VR
support and motion interactions to that existing game installation.

**No Blizzard game assets are included in the ZIP.** It contains mod files
and licensed third-party components. Required item/skill icons and potion
models are read from your own installed game into local caches during setup;
do not redistribute those caches or your game files.

## Headset connections

- **Steam Frame:** SteamVR streaming.
- **Quest 3:** Steam Link with SteamVR, or Virtual Desktop with VDXR.
- **Quest 2:** the same Quest connection options; compatibility is not yet confirmed.

The game runs on your Windows PC and streams to your headset.

## What's new

- Physical basic-javelin throwing: equip a javelin in **WEAPON** mode, hold
  right trigger, make a throwing motion and release. The game handles ammunition
  and damage; free jabs work with the trigger released.
- Shoulder weapon swapping now uses **grip**, leaving trigger available for throwing.
- Terrain-correct bow and javelin contacts across cave transitions.
- Native arrow artwork aligned to the tracked draw guide and release trajectory,
  with silent preview creation.
- Corrected arrow and javelin model facing so their tips point along their flight.
- Updated controller instructions in the README, ZIP and VR settings.

## Features

- First-person VR with motion controls, free-swing melee, aimed spells, bows
  and physical javelin throws.
- Guided Windows installation, headset/connection selection and automatic
  Steam library shortcuts. Update / repair keeps calibration and video settings.
- Body calibration for your height and arm span on first load, or through
  **Left Grip + Frame Left B / Quest Left Y → Calibrate body**.
- Weapon-set swapping by reaching your right hand over either shoulder and
  pressing its grip; the swap hand is adjustable in VR settings.
- Physical belt potions and wrist health/mana indicators.
- Inventory, skills, conversations and Journal through the tabbed game menu:
  **Frame Left A / Quest Left X**.
- Native menu buttons and sticks alongside laser selection and inventory dragging.
  Native **X** drops an item; **press and hold right-stick click** sorts inventory.
- A complete **[controller reference](CONTROLS.md)** in the README and ZIP.

## Download, install and play

1. Download **d2r-pcvr-windows.zip** under **Assets** below.
2. Extract `d2r-pcvr-preview` to a permanent folder.
3. Double-click **Install-Windows.cmd** and follow the setup wizard. It creates
   the Steam shortcuts for your chosen headset and connection.
   See **INSTALL-WINDOWS.md** for help or manual setup.
4. Connect your headset, then double-click the matching launcher:
   - **Launch-VR-SteamVR.cmd** for SteamVR / Steam Link.
   - **Launch-VR-VDXR.cmd** for Virtual Desktop, with SteamVR closed.
5. Accept Steam's launch prompt if shown and select an **offline character**.

To launch from Steam or the headset, use the shortcuts created by setup.
Manual directions are under **Add Steam library shortcuts** in **INSTALL-WINDOWS.md**.
Use the matching entry for SteamVR or Virtual
Desktop; keep SteamVR closed when using Virtual Desktop/VDXR.

**Upgrading from an earlier preview?** Follow **Upgrade while keeping your settings** in
**INSTALL-WINDOWS.md** to retain body calibration and video settings.

Keep the extracted folder after installation. For ordinary desktop play,
fully exit VR and use Steam's normal **Play** button. Use the unmodded game
for multiplayer.
