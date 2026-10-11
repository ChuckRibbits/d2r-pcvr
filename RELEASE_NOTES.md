# D2R PCVR v0.3.2 — Windows preview

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

- Native menu button and stick navigation alongside laser selection and dragging.
- Quest and Steam Frame mappings for the game's displayed menu prompts; laser
  context-clicks use **Left Grip + Right Trigger**.
- A shared background in both eyes for stash, standalone inventory and other
  item-management panels.
- A complete **[controller reference](CONTROLS.md)** in the README and ZIP,
  covering gameplay, quick-cast shortcuts, laser controls and native menu buttons.
- Clear purchased-game requirements: this is a mod, and no game assets are included.
- Free melee swings stay available with a wheel skill selected. Casting,
  menus and potion handling still pause swing damage.
- More reliable contact from fast weapon swings, including in caves.
- Instructions for separate Steam library shortcuts for **SteamVR** and
  **Virtual Desktop**, so you can launch without opening the mod folder.

## Features

- First-person VR with motion controls, free-swing melee, aimed spells and bows.
- Body calibration for your height and arm span on first load, or through
  **Left Grip + Frame Left B / Quest Left Y → Calibrate body**.
- Weapon-set swapping by reaching your right hand over either shoulder and
  pulling its trigger; the swap hand is adjustable in VR settings.
- Physical belt potions and wrist health/mana indicators.
- Inventory, skills, conversations and Journal through the tabbed game menu:
  **Frame Left A / Quest Left X**.

## Download, install and play

1. Download **d2r-pcvr-windows.zip** under **Assets** below.
2. Extract `d2r-pcvr-preview` to a permanent folder.
3. Open **INSTALL-WINDOWS.md** in that folder and follow the setup steps.
   You will need **Python 3.10 or newer**; the installer is included.
4. Connect your headset, then double-click the matching launcher:
   - **Launch-VR-SteamVR.cmd** for SteamVR / Steam Link.
   - **Launch-VR-VDXR.cmd** for Virtual Desktop, with SteamVR closed.
5. Accept Steam's launch prompt if shown and select an **offline character**.

To launch from Steam or the headset, follow **Add Steam library shortcuts**
in **INSTALL-WINDOWS.md**. Use its separate entries for SteamVR and Virtual
Desktop; keep SteamVR closed when using Virtual Desktop/VDXR.

**Upgrading from an earlier preview?** Follow **Upgrade while keeping your settings** in
**INSTALL-WINDOWS.md** to retain body calibration and video settings.

Keep the extracted folder after installation. For ordinary desktop play,
fully exit VR and use Steam's normal **Play** button. Use the unmodded game
for multiplayer.
