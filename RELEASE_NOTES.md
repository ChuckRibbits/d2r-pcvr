# D2R VR Unleashed v0.3.26 - Windows preview

This hotfix fixes being stuck after a knockback, such as Duriel's stun in Act 2.
It includes everything from
[v0.3.25](https://github.com/bearstonem/d2r-pcvr/releases/tag/v0.3.25-windows-preview):
weapon skills on contact, throwing by hand, Druid shapeshifting, smarter aiming,
physics and experimental frame generation.

## Gameplay and combat

- **Knockbacks no longer lock you in place - contributed by ChuckRibbits:**
  After being knocked back, for example by Duriel's Smite or another attack that
  stuns, you could no longer move or interact with anything until you saved and
  exited. A knockback now ends normally, and you can walk again as soon as it
  does.

## Updating

Download **D2R-PCVR-Setup-0.3.26.exe** for graphical installation, or
**d2r-pcvr-windows.zip** for portable setup. Existing users should choose
**Update / repair and keep calibration and video settings**.

Close the game and fully exit Steam before updating. Closing D2R VR Launcher
also avoids file locks. Update / repair preserves settings, calibration and saves.
A full install applies recommended defaults and resets calibration.

On native Windows, Setup downloads OFXR Bridge for frame generation, so keep the
internet connection on. If that download fails, everything else still installs
and frame generation reports that it is not installed. Frame generation also
needs the Microsoft Visual C++ x64 runtime.

The Windows installer includes **D2R VR Launcher 0.3.5**, installing or updating
it when needed while preserving newer versions. Direct Steam shortcuts and
portable setup remain available; Authentic is a separate optional mod.

### First-time setup and using the launcher

1. Install and start the original Diablo II: Resurrected through Steam or
   Battle.net once. Complete its account prompts, create or select an offline
   character, then fully exit the game.
2. Download **D2R-PCVR-Setup-0.3.26.exe** from the assets below. Fully exit
   Steam using **Steam > Exit**, then run Setup and choose **Full install with
   recommended settings**. Follow the wizard to select your game copy, headset
   and connection; the shared launcher is included.
3. After Setup finishes, reopen Steam and reconnect your headset.
4. Open **Start > D2R VR Launcher**, confirm the **Game folder** and select
   **D2R VR Unleashed**.
5. Choose **SteamVR** for Steam Frame or Quest through Steam Link/SteamVR.
   Choose **Virtual Desktop (VDXR)** for Quest through Virtual Desktop, with its
   Quest app and Windows Streamer running and SteamVR closed. The runtime choice
   applies to this launch without changing your Windows-wide OpenXR setting.
6. Click **Play**, select a **local offline character** and complete body
   calibration when prompted.

### Current log location

For support, reproduce the problem, close the game and send these logs from
your game folder. In Steam's default library that is
`C:\Program Files (x86)\Steam\steamapps\common\Diablo II Resurrected`:

```text
mods\D2R-PCVR\d2rloader\logs\d2r-vr-frame.log
mods\D2R-PCVR\d2rloader\logs\d2r-pcvr-vrcam.log
mods\D2R-PCVR\reshade\ReShade.log
d2rloader\logs\d2rloader.log
```

The `d2r-vr-frame.log` and `d2r-vr-vrcam.log` in the game's main
`d2rloader\logs` folder are left over from older versions and are no longer
updated. Developer mode uses `mods\pcvr-dev` in place of `mods\D2R-PCVR`.
Installer failures are recorded separately in
`%LOCALAPPDATA%\Programs\D2R PCVR\setup-install.log`.

## Verification and known issues

The release build passed **140 native checks**. **250 of 255 Python checks**
passed on Windows: the 4 session checks need Linux's `/proc`, and 1 symbolic-link
check was skipped because this Windows session cannot create symbolic links. The
in-headset Notes pages were rendered and inspected for fit and content.

The knockback fix was tested on a headset before it was merged: a level 90
character in Duriel's Lair took 9 knockbacks in about 50 seconds, and every one
ended normally, with the hero walking again about 0.8 seconds after the stun
appeared. Melee kept working through the fight. The lock itself was not
reproduced on an unfixed build; it comes from a player's report. No new headset
session is claimed for the version and release-notes update itself.

The v0.3.25 features were tested with offline characters on Steam D2R 3.3.94995
through SteamVR on a Steam Frame, in the isolated developer environment, as they
were merged:
- weapon skills on contact;
- throwing potions;
- the Notes tab's pages;
- frame generation with FidelityFX and NVIDIA, including menus and panels, and
  pausing and resuming when switching between Both eyes and By turns.

Contributed features were tested by their authors before review.

Known issues:
- **Stuns in Unleashed:** a stun now only knocks you back. You can move again as
  soon as the knockback ends, even while the stun is still shown.
- **Frame generation is experimental.** It uses a pre-release bridge. Expect
  artifacts around moving monsters and during fast head turns. Frame counters,
  including the Perf page's, count only the frames the game draws. Once, with it
  on, the Escape menu fell back to the flat panel.
- **Throwing potions:** the burst follows the game's short throw animation, and
  only right-hand potions are thrown by hand for now.
- **By turns:** it still renders each eye on alternate frames. Transparent
  effects and wispy hair without useful depth may keep artifacts.
- **Sorceress elemental hands:** they are drawn over the picture, so they can show
  through your weapon or a wall.
- **Not retested:** the latest rendering changes have not been retested through
  VDXR or on other GPU vendors. Battle.net remains experimental.

## Special thanks

Thanks to **beadlebytes** for smarter aiming, ground skills, the bow path laser,
the forearm and shield, wrist map zoom and centered auras. Thanks to
**ChuckRibbits** for Druid shapeshifting, the strike framework, Unleashed
combat and this knockback fix. Thanks to **Mcklem** for the mercenary, enemy sight, torches and the
Kurast Docks fix. Thanks to **tig3rmast3r** for OFXR Bridge, and to everyone
testing, reporting bugs and helping improve the mod!

D2R VR Unleashed is a mod by **monkeylashes**, with contributions from the community.

## Downloads and links

- [Windows installer, portable ZIP and checksums](https://github.com/bearstonem/d2r-pcvr/releases/tag/v0.3.26-windows-preview)
- [Installation instructions](https://github.com/bearstonem/d2r-pcvr/blob/v0.3.26-windows-preview/INSTALL-WINDOWS.md)
- [Controls](https://github.com/bearstonem/d2r-pcvr/blob/v0.3.26-windows-preview/CONTROLS.md)
- [Full source comparison](https://github.com/bearstonem/d2r-pcvr/compare/v0.3.25-windows-preview...v0.3.26-windows-preview)
- [Project on GitHub](https://github.com/bearstonem/d2r-pcvr)
- [Discord](https://discord.gg/DmbFfJAnnm)
- [Support](https://ko-fi.com/bearstonem)

<!-- PCVR_IN_HEADSET_NOTES
release: v0.3.26-windows-preview
credits: ChuckRibbits; beadlebytes; Mcklem; tig3rmast3r (OFXR Bridge); Sergious.
- Hotfix: a knockback (Duriel's Smite and others) no longer leaves you unable to move.
- Included from v0.3.25: the features below.
- Weapon skills cast on contact: put Bash, Zeal, Charged Strike and more on a trigger and swing.
- Skill picker: GESTURE marks contact skills; hover any skill for a card on how it works.
- Druid: first-person Werewolf/Werebear; claws cast Maul, Fury and the other form skills.
- Druid: transform with your body - arms crossed then flung out, or hands up then slammed down.
- Melee is Unleashed only; no flinch when struck; on-hit effects only on a stroke's first contact.
- Guard strain: black vignette with an On/Off switch; reaches the edge of wide headsets.
- Throw knives, axes and every javelin by hand; javelin skills and Double Throw ride the throw.
- Assassin: throw Fire Blast, Shock Web, sentries and Wake skills to place them.
- Gas and oil potions are thrown by hand as real bottles; the burst lands where they shatter.
- Aim: each hand and your eyes pick a monster; spells and bow skills can go to it.
- Ground skills land where you point or on the aimed monster, with timed landing circles.
- Bow path laser at full draw (Aim, off by default); Throw lift sets how high throws leave.
- Mercenary: Enhanced VR AI keeps it in view, follows on a leash and frees itself (Combat).
- Enemy sight distance: offline, monsters notice you from 100% up to 300% (Combat).
- Torch clubs light the dark from their head; Video > Torch flame hides the flame only.
- Right stick: push up to toggle run/walk; hold down for loot labels.
- Wrist map: look at it and push the right stick to zoom; Map view adds the full map.
- Forearm turns with the wrist; Shield tab positions a held shield on the forearm.
- Quest and level-up notices appear on a card in the room, not at your eyes.
- VR Settings shows only rows for your character; Notes tab pages long release notes.
- Experimental frame generation (Perf, Both eyes only): OFXR Bridge, FidelityFX or NVIDIA.
- Physics: emptied bottles roll; weapons knock bottles, cups, plates and vases about.
- Sharper By turns, without frame-old ghosts beside moving monsters.
- Sorceress elemental hands (experimental, Video); Paladin auras centered; Kurast canoe fixed.
PCVR_IN_HEADSET_NOTES -->
