# D2R PCVR v0.3.9 — Windows preview

Play **Diablo II: Resurrected in VR on Windows** with
**SteamVR or Virtual Desktop (VDXR)**. This preview is for **local offline
single-player** and accepts your **Steam copy of D2R 3.3.93787** or
**Battle.net copy of D2R 3.3.93847** (experimental).

**This download is a mod, not the original game. You must purchase and install
your own legitimate copy of Diablo II: Resurrected.** D2R PCVR adds VR
support and motion interactions to that existing game installation.

**No Blizzard game assets are included in the installer or ZIP.** They contain mod files
and licensed third-party components. Required item/skill icons and potion
models are read from your own installed game into local caches during setup;
do not redistribute those caches or your game files.

## Headset connections

- **Steam Frame:** SteamVR streaming.
- **Quest 3:** Steam Link with SteamVR, or Virtual Desktop with VDXR.
- **Quest 2:** the same Quest connection options; compatibility is not yet confirmed.

The game runs on your Windows PC and streams to your headset.

Battle.net has been confirmed with SteamVR. Battle.net with Virtual Desktop /
VDXR is available but has not yet been confirmed. The Steam desktop client
is required for library shortcuts, including with the Battle.net edition.
**Battle.net owners do not need to purchase a second copy through Steam.**

## What's new

- **Installer error details:** the graphical wizard now shows the mod installer's
  progress and actual error, and saves full output to `setup-install.log` beside
  `setup-last.log` in `%LOCALAPPDATA%\Programs\D2R PCVR`. This fixes missing
  diagnostics in [issue #11](https://github.com/bearstonem/d2r-pcvr/issues/11);
  the reporter's underlying installation failure still needs their detailed log.
- **Steam VR shortcut startup:** Steam-edition VR launches now use a Windows
  desktop handoff to avoid the launcher chain associated with early loader
  crashes. The existing SteamVR and Virtual Desktop shortcuts still work;
  the selected runtime and game-session lifetime are preserved.
- **Physical Paladin Smite:** select Smite, then swing or thrust the left shield
  through an enemy with right trigger released. The shield follows the left wrist
  independently of the main weapon. Native Smite costs, reach and timing apply.
- **Amazon bow skills:** draw and release once to request the selected arrow skill.
  Multiple Shot retains its fan and Strafe its native volley; other supported
  arrows retain native effects, costs and cadence. Specialized arrows still use
  native animation and ground-plane flight/collision.
- **Active skill cues:** the wheel marks the selected skill with a gold ACTIVE
  badge, and the weapon/shield shows a skill cue. Unavailable motion activation
  shows TRIGGER ONLY so the button fallback is clear.
- **Map in the spell menu:** hold left secondary, point with the right laser and
  select **MAP** in the bottom tabs. **SPELLS** returns to skills without changing
  the selected skill; releasing left secondary closes the menu. Wrist health,
  mana, level and XP remain.

Run **D2R-PCVR-Setup-0.3.9.exe** for graphical setup, or use the portable ZIP.
The EXE includes a private Python runtime and creates the matching shortcuts.
Choose **Update / repair and keep calibration and video settings** when upgrading;
keep existing setup backups. Normal desktop Play and Windows removal are retained.

Use the game's **left or right corner minimap** layout for the Map tab.
Full-screen automap is available through the game's normal map controls.

## Validation and remaining checks

The native Windows build passes **40 CTest checks**, and **69 Python checks**
cover setup, repair/removal, rollback, Battle.net, launcher lifecycle and installer
payload staging. The owner previously confirmed physical Paladin shield bash on
Steam Frame / SteamVR with Steam 3.3.93787. Four controlled desktop-handoff
launches reached the frontend and initialized OpenXR; the first three closed
normally, and the fourth was left open for testing.

The compiled Inno Setup lifecycle fixture also passes installation, Windows Apps
registration, helper retention after a blocked removal, and successful removal.
This fixture uses a stub backend and does not modify a real game installation.

A fresh owner-triggered launch from inside Frame, Map-tab placement in both eyes,
Amazon arrow skills, shield fit across models and cancellation during native
wind-up still need headset checks. The new installer diagnostics have been tested
with real hidden Windows subprocesses and disposable installation fixtures;
the reporter's PC has not been retested. Quest 2 and Battle.net VDXR remain
unconfirmed.

## Features

- First-person VR with motion controls, free-swing melee, aimed spells, bows
  and physical javelin throws.
- Guided Windows installation, headset/connection selection and automatic
  Steam library shortcuts. Update / repair keeps calibration and video settings.
- Body calibration for your height and arm span on first load, or through
  **Left Grip + Frame Left B / Quest Left Y → Calibrate body**.
- Weapon-set swapping by reaching your right hand over either shoulder and
  pressing its grip; the swap hand is adjustable in VR settings.
- Physical belt potions and wrist health, mana, level and experience indicators,
  plus a spell-menu Map tab showing explored areas and native markers.
- Item pickup, chests, shrines and cave-entrance interactions through the
  right A button.
- Inventory, skills, conversations and Journal through the tabbed game menu:
  **Frame Left A / Quest Left X**.
- Native menu buttons and sticks alongside laser selection and inventory dragging.
  Native **X** drops an item; **press and hold right-stick click** sorts inventory.
- A complete **[controller reference](CONTROLS.md)** in the README and ZIP.

## Download, install and play

First launch your purchased game normally through its store, finish the account
prompts, load an offline character, then use **Save & Exit** and fully close D2R.

For graphical setup, download **D2R-PCVR-Setup-0.3.9.exe**, run it and follow the
wizard. Fully exit Steam when asked. Reopen Steam, connect your headset and use
the matching VR shortcut with an offline character.

For portable setup:

1. Download **d2r-pcvr-windows.zip** under **Assets** below.
2. Extract `d2r-pcvr-preview` to a permanent folder.
3. Double-click **Install-Windows.cmd** and follow the setup wizard. It creates
   the Steam shortcuts for your chosen game edition, headset and connection.
   See **INSTALL-WINDOWS.md** for help or manual setup.
4. Open Steam, connect your headset and use the matching **SteamVR** or
   **Virtual Desktop** library entry. Battle.net entries include **(Battle.net)**.
   Keep SteamVR closed for Virtual Desktop / VDXR.
5. Accept Steam's launch prompt if shown and select an **offline character**.

To launch from Steam or the headset, use the shortcuts created by setup.
The included **Launch-VR-*.cmd** files are for the **Steam edition**;
Battle.net players should use the **(Battle.net)** library entries. These
launch the mod directly, so the Battle.net app may not show it as in progress.
Manual directions are under **Add Steam library shortcuts** in **INSTALL-WINDOWS.md**.
Use the matching entry for SteamVR or Virtual
Desktop; keep SteamVR closed when using Virtual Desktop/VDXR.

**Upgrading from an earlier preview?** Follow **Upgrade while keeping your settings** in
**INSTALL-WINDOWS.md** to retain body calibration and video settings.

Keep the installed helper folder or extracted ZIP folder after installation. For ordinary desktop play,
fully exit VR and use the original Steam or Battle.net **Play** button.
Use the unmodded game for multiplayer. Keep **setup-backups**, including
backups in older extracted release folders, until removal is complete.

Read **INSTALL-WINDOWS.md** and **CONTROLS.md** in the helper folder or ZIP, or open the
[installation guide](https://github.com/bearstonem/d2r-pcvr/blob/v0.3.9-windows-preview/INSTALL-WINDOWS.md)
and [controller reference](https://github.com/bearstonem/d2r-pcvr/blob/v0.3.9-windows-preview/CONTROLS.md).
