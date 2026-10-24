# D2R VR Unleashed v0.3.25 - Windows preview

This update focuses on physical combat skills and throwing, Druid shapeshifting,
smarter aiming, objects you can knock about, and experimental frame generation.

## Gameplay and combat

- **Weapon skills strike on contact:** Put a weapon skill on a trigger and swing.
  The skill casts when the weapon connects:
  - Barbarian: Bash, Stun, Concentrate and Berserk.
  - Paladin: Sacrifice, Vengeance, Zeal and Conversion.
  - Amazon: Power Strike, Impale, Charged Strike and Lightning Strike.
  - Assassin: Poison Dagger.

  The skill picker marks these skills **GESTURE**. Hovering any skill now shows
  a card explaining how it works, including the maths behind basic attacks.
  Built on ChuckRibbits' strike framework.
- **Druid shapeshifting in first person - contributed by ChuckRibbits:**
  Werewolf and Werebear forms have a first-person beast body and eye height. The
  claws strike physically and cast the form skills: Maul, Feral Rage, Rabies,
  Fire Claws, Hunger and Fury. To transform with your body:
  - **Werewolf:** cross your arms over your chest, bow, then fling them up and out.
  - **Werebear:** hold both hands above your head, then slam them down.
- **Unleashed combat only:** Melee always uses Unleashed rules, and the Classic
  option is gone. A saved Classic choice is ignored. Combat changes, contributed
  by **ChuckRibbits**:
  - The hero no longer flinches when struck.
  - A weapon's on-hit effects (crushing blow, open wounds, leech, chance to cast)
    roll only on a full stroke's first contact.
  - An attack's other effects (chill, stun, drain, thorns) apply only when its
    normal hit roll would have hit.
- **Guard strain - contributed by ChuckRibbits:** The guard-strain vignette is
  black, with a simple On/Off switch under **VR Settings > Combat > Guard strain
  vignette**. It now reaches the edge of wide headset views.
- **Throwing by hand:** Throw throwing knives, axes and every javelin with your
  arm.
  - Javelin skills ride the throw: Poison Javelin, Lightning Bolt, Plague Javelin
    and Lightning Fury.
  - A Barbarian's Double Throw throws each hand's own weapon.
  - **VR Settings > Aim > Throw lift** sets how far above the hand's path throws
    leave.
  - Assassins throw Fire Blast, Shock Web, the sentries and Wake skills to place
    them where the arc lands.
  - beadlebytes restored javelin motion throws.
- **Throwing potions:** A gas or oil potion in the right weapon slot is thrown by
  hand as a real bottle. The game's burst goes off where it shatters.
- **Smarter aiming - contributed by beadlebytes:** Each hand and your eyes pick a
  monster. Everything under **VR Settings > Aim** is off by default. There you
  can choose:
  - whether spells and bow skills go to the aimed monster;
  - whether each hand's monster is outlined in its own color;
  - where ground skills land: Blizzard, Meteor, Fire Wall, Hydra, Fissure,
    Volcano, Bone Wall, Bone Prison and traps can land where you point or on the
    aimed monster, with circles that show where and when they will hit.
- **Bow path laser - contributed by beadlebytes:** At full draw, an optional faint
  line shows exactly where the arrow will fly (**VR Settings > Aim > Bow path
  laser**, off by default).
- **A smarter mercenary - contributed by Mcklem:**
  - stays where you can see it;
  - adjusts how far it detects enemies during a fight;
  - follows on an elastic leash;
  - frees itself when stuck.

  It is on by default under **VR Settings > Combat > Enhanced VR Mercenary**.
  Legacy restores the game's behavior.
- **Enemy sight distance - contributed by Mcklem:** Offline, monsters can notice
  you from farther away, from the game's 100% up to 300% (**VR Settings >
  Combat**).
- **Torches - contributed by Mcklem:** One-handed clubs with fire damage act as
  torches that light the dark from the club's head. The light radius is 18 by
  default. **VR Settings > Video > Torch flame** hides the flame but keeps the
  light.

## Controls and interface

- **Right stick up and down:** Push up to toggle running and walking. Hold down
  to show nearby loot labels. This replaces the Right Grip + Right Trigger chord.
- **Wrist map zoom - contributed by beadlebytes:** Look at the wrist map and push
  the right stick up or down to zoom, up to 16 times. **Map view** adds the
  game's full map, isometric or top-down.
- **Forearm and shield - contributed by beadlebytes:** The forearm now turns with
  the wrist. **VR Settings > Shield** positions a held shield on the forearm:
  grip offset, out, up, how far it follows the wrist, and roll.
- **Quest and level-up notices** now appear on a card in the room, not in the
  middle of your view.
- **Tidier VR Settings:** Pages show only the rows that apply to your character
  and weapon, and related settings share a row. The **Notes** tab pages through
  long release notes.

## Visuals and stability

- **Frame generation (experimental):** **VR Settings > Perf > Frame generation**
  runs [OFXR Bridge](https://github.com/tig3rmast3r/OFXR-Bridge) by tig3rmast3r.
  The game draws every other headset frame and the bridge generates the rest.
  - **Choices:** FidelityFX (any graphics card) or NVIDIA (RTX or GTX 1660
    family), with NVIDIA's quality and scale on a row below.
  - **Requirements:** Stereo rendering: **Both eyes** (it pauses with By turns)
    and native Windows.
  - Setup downloads the bridge. It is off by default, and turning it on or off
    takes a restart.
  - The bottom line of the Perf page shows what it is doing.
- **Physics you can knock about:**
  - An emptied potion bottle falls, bounces and rolls.
  - Weapons, fists and the shield knock bottles and the level's small props
    (cups, plates, vases, books, barrels and more) about.
  - Thrown potions, traps and bombs stop at walls and tables.

  The physics is purely visual. Switch it under **VR Settings > Video > Bottle
  physics** and **Knockable props**.
- **Sharper By turns:** The eye not drawn is noticeably sharper, and moving
  monsters no longer leave a frame-old ghost beside them.
- **Sorceress elemental hands (experimental):** Each hand shows a small effect
  made from its spell's own textures (**VR Settings > Video**, off by default).
- **Centered Paladin auras - contributed by beadlebytes:** Aura rings now sit
  around the hero's feet in VR.
- **Kurast Docks - contributed by Mcklem:** Removed the canoe floating above the
  docks.
- **Latest game update:** Setup accepts Steam D2R 3.3.94995 (the October 22
  update).

## Updating

Download **D2R-PCVR-Setup-0.3.25.exe** for graphical installation, or
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
2. Download **D2R-PCVR-Setup-0.3.25.exe** from the assets below. Fully exit
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

For support, reproduce the problem and send the current gameplay log from
inside your game folder:

```text
mods\D2R-PCVR\d2rloader\logs\d2r-vr-frame.log
```

Developer mode uses `mods\pcvr-dev\d2rloader\logs\d2r-vr-frame.log`.
Installer failures are recorded separately in
`%LOCALAPPDATA%\Programs\D2R PCVR\setup-install.log`.

## Verification and known issues

The release build passed **140 native checks**. **250 of 255 Python checks**
passed on Windows: the 4 session checks need Linux's `/proc`, and 1 symbolic-link
check was skipped because this Windows session cannot create symbolic links. The
in-headset Notes pages were rendered and inspected for fit and content.

The merged features were tested with offline characters on Steam D2R 3.3.94995
through SteamVR on a Steam Frame, in the isolated developer environment, as they
were merged:
- weapon skills on contact;
- throwing potions;
- the Notes tab's pages;
- frame generation with FidelityFX and NVIDIA, including menus and panels, and
  pausing and resuming when switching between Both eyes and By turns.

Contributed features were tested by their authors before review. These are
feature-level checks; no new headset session is claimed for the version and
release-notes update itself.

Known issues:
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
**ChuckRibbits** for Druid shapeshifting, the strike framework and Unleashed
combat. Thanks to **Mcklem** for the mercenary, enemy sight, torches and the
Kurast Docks fix. Thanks to **tig3rmast3r** for OFXR Bridge, and to everyone
testing, reporting bugs and helping improve the mod!

D2R VR Unleashed is a mod by **monkeylashes**, with contributions from the community.

## Downloads and links

- [Windows installer, portable ZIP and checksums](https://github.com/bearstonem/d2r-pcvr/releases/tag/v0.3.25-windows-preview)
- [Installation instructions](https://github.com/bearstonem/d2r-pcvr/blob/v0.3.25-windows-preview/INSTALL-WINDOWS.md)
- [Controls](https://github.com/bearstonem/d2r-pcvr/blob/v0.3.25-windows-preview/CONTROLS.md)
- [Full source comparison](https://github.com/bearstonem/d2r-pcvr/compare/v0.3.24-windows-preview...v0.3.25-windows-preview)
- [Project on GitHub](https://github.com/bearstonem/d2r-pcvr)
- [Discord](https://discord.gg/DmbFfJAnnm)
- [Support](https://ko-fi.com/bearstonem)

<!-- PCVR_IN_HEADSET_NOTES
release: v0.3.25-windows-preview
credits: beadlebytes; ChuckRibbits; Mcklem; tig3rmast3r (OFXR Bridge); Sergious.
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
