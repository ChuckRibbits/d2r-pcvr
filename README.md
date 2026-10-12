# D2R PCVR

Play **Diablo II: Resurrected in VR on Windows**, with support for
**SteamVR and Virtual Desktop (VDXR)**. Use motion controls to swing your
weapon, aim spells, drink belt potions and explore Sanctuary in first person.

[![Watch the D2R PCVR introduction on YouTube](https://github.com/bearstonem/d2r-pcvr/blob/main/docs/media/d2r-pcvr-introduction.jpg?raw=true)](https://www.youtube.com/watch?v=RpqVcaseReI)

**[Watch the introduction on YouTube](https://www.youtube.com/watch?v=RpqVcaseReI)** (10:27):
physical bow and melee combat, over-the-shoulder weapon swaps, raising
skeletons, aimed spells, belt potions and VR menus, recorded in the headset.

D2R PCVR builds on **[D2R VR by BodyWalkVR](https://github.com/natpoh/diablo2-vr-port)**,
the original VR mod for Diablo II: Resurrected. See [Credits](#credits).

**This is a mod for the original game, not a standalone game. You must purchase
and install your own legitimate copy of Diablo II: Resurrected.** The mod adds
VR support and motion interactions on top of your existing game installation.

The **v0.3.7 Windows preview** supports **Steam 3.3.93787** and experimental
**Battle.net 3.3.93847** installations.
Run **Install-Windows.cmd** and choose **Steam** or **Battle.net** before choosing
your headset and connection. See
[Battle.net preview installation](INSTALL-WINDOWS.md#battlenet-preview-packages).
Battle.net has been confirmed with SteamVR; Battle.net with Virtual Desktop / VDXR
has not yet been confirmed.

**No Blizzard game assets are included in this repository or the release ZIP.**
The download contains the mod and licensed third-party components, not the
game, its executable, models, textures or audio. During installation, required
item/skill icons and potion models are read from your own installed game into
local caches. Keep those caches local; do not include them when sharing the mod.

**[Download the Windows preview](https://github.com/bearstonem/d2r-pcvr/releases/tag/v0.3.7-windows-preview)**
and follow **[Install and play](INSTALL-WINDOWS.md)**.
Download **d2r-pcvr-windows.zip** from the release's Assets section.
Extract it to a permanent folder, then double-click **Install-Windows.cmd**.
The setup wizard installs the mod, lets you choose your headset and connection,
and adds the matching Steam library shortcuts. It asks which game edition to
use and detects its installation folder.
No separate Python install is needed.
The ZIP also includes **Uninstall-Windows.cmd**. It keeps
your current characters and shared stash, restores files from the installation
backups and removes the selected edition's VR shortcuts. Keep the installation
backups, including those in older release folders, until removal is complete.

## What you need

- A Windows PC with your **purchased Steam copy of D2R 3.3.93787**, or
  **Battle.net copy of D2R 3.3.93847** (experimental support).
- The Steam desktop client, including for Battle.net VR library shortcuts.
- A VR headset and two tracked controllers.
- **SteamVR**, or **Virtual Desktop for Quest with its Windows Streamer**.
- Internet access during setup to download the required components.
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
- Free-swing melee, including with a wheel skill selected, aimed spells, bows and physical javelin throws.
- Belt potions you can grab and drink, plus wrist health, mana, level and experience progress.
- A small square automap above the wrist indicators, showing the game's explored map and markers.
- Body calibration for your height and arm span, available on first load
  and through **Left Grip + left secondary → Calibrate body**.
- Weapon-set swapping: reach your right hand over either shoulder and press
  its grip. You can change the swap hand in VR settings.
- Inventory, skills, conversations and Journal through the game menu:
  **Frame Left A / Quest Left X**.

[Watch the wrist XP and map demo (12 seconds)](https://github.com/bearstonem/d2r-pcvr/releases/download/v0.3.6-windows-preview/d2r-pcvr-wrist-hud-demo.mp4).

## Controller reference

Windows preview for **SteamVR and Virtual Desktop (VDXR)**. These controls
use two tracked VR controllers; an Xbox controller is not required.

### Button names

| Hand / button | Quest 2 / 3 label | Steam Frame label |
| --- | --- | --- |
| Right primary / secondary | A / B | A / B |
| Left primary / secondary | X / Y | A / B |
| Trigger | Index trigger | Index trigger |
| Grip | Grip button | Grip input |
| Stick click | Press the thumbstick | Press the thumbstick |

**Left primary** below means **Quest Left X / Frame Left A**.
**Left secondary** means **Quest Left Y / Frame Left B**.

### Gameplay

| Action | VR control |
| --- | --- |
| Walk | Left stick |
| Snap turn | Right stick left / right |
| Interact / pick up / talk | Right A while aiming at the target |
| Open the tabbed game menu | Left primary; select Inventory, Character, Skills or Journal in its tabs |
| Spell wheel | Hold left secondary and turn your left palm toward you; select with right laser + trigger |
| Assign wheel skills | Wheel → **EDIT SKILLS** → slot → learned skill |
| Select basic weapon mode | Choose **WEAPON** in the wheel center, then release the trigger |
| Free melee swing / thrust | Move your equipped melee weapon through an enemy; also works with a wheel skill selected |
| Manual attack / selected skill | Right trigger; hold for the skill's native repeat behavior |
| Bow | Bring the right hand to the bowstring, hold right trigger, draw back, then release |
| Javelin throw | Javelin equipped in **WEAPON** mode: hold right trigger, make a throwing motion, then release; direction follows the hand's motion |
| Cancel a grab / bow draw / javelin throw | Right B, then release the inputs |
| Swap weapon sets I / II | Reach the swap hand over either shoulder and press its grip; right hand by default, adjustable in VR settings |
| Seated weapon selection / stow | Wheel → **EQUIPMENT / BELT** → **Weapon I**, **Weapon II** or **Stow/Draw** |
| Grab a belt potion | Reach to a bottle and hold that hand's grip |
| Drink | Keep grip held, bring the bottle opening to your mouth and tip it; release an unused bottle to return it |
| Quick potion | Wheel → **EQUIPMENT / BELT** → select a potion with right trigger |
| Wrist health / mana / XP / map | Look at the back of your left wrist, like checking a watch |
| Map | Left stick click |
| Reset height | Left Grip + left stick click, during gameplay |
| VR settings | Left Grip + left secondary; also **VR SETTINGS** in the wheel |
| Calibrate height / arm span | VR settings → **Calibrate body**; stand straight in a T-pose, release both triggers, then hold both together until **Saved** |
| Recenter | PC F11 |
| Save stereo images | PC F8 |

Free swings pause during casting, menus and potion handling. Selecting a wheel
skill changes the right-trigger action; its quick-cast binding stays available.
The belt shows potions in the game's belt slots even without an equipped belt.

The wrist shows your level and progress toward the next level beneath the
health/mana orbs. Its square map uses the game's **left or right corner minimap**
layout. Raising your wrist temporarily opens the automap if needed; lowering
it restores its previous visibility. Only explored areas and native markers
appear. Left stick click still toggles the normal map.

Javelins use the game's normal basic throw, ammunition and damage. A stationary
trigger release does not throw. Jabs work with the trigger released. If a wheel
skill is selected, right trigger casts that skill; choose **WEAPON** to return
to physical throwing, or use the quick-cast shortcuts for skills. Physical
throwing currently covers basic javelins; skill projectiles retain their normal
casting controls.

### Quick-cast shortcuts

Hold **Left Trigger**, aim the right hand, then press the assigned face button.
Add **Left Grip** for slots 5–8. Assign the eight slots through **EDIT SKILLS**;
the wheel shows each slot's VR shortcut. Release the face button before changing
slots or the grip modifier.

| Wheel slot | Quest shortcut | Steam Frame shortcut |
| --- | --- | --- |
| 1 | Left Trigger + Right A | Left Trigger + Right A |
| 2 | Left Trigger + Right B | Left Trigger + Right B |
| 3 | Left Trigger + Left X | Left Trigger + Left A |
| 4 | Left Trigger + Left Y | Left Trigger + Left B |
| 5 | Left Trigger + Left Grip + Right A | Left Trigger + Left Grip + Right A |
| 6 | Left Trigger + Left Grip + Right B | Left Trigger + Left Grip + Right B |
| 7 | Left Trigger + Left Grip + Left X | Left Trigger + Left Grip + Left A |
| 8 | Left Trigger + Left Grip + Left Y | Left Trigger + Left Grip + Left B |

### Menus: laser and native buttons

Both methods work in the main menu and native game panels, including inventory,
stash and merchants. Keep the D2R window focused on the PC.

Use a **stick or face button** to start native button navigation and follow the
game's displayed prompts. The idle laser will not move your native selection.
To return to the laser, release the native controls and deliberately move your
aim across the panel. A small hand tremor will not change methods.

#### Laser controls

| Action | VR control |
| --- | --- |
| Hover / point | Aim the right controller's laser |
| Select / drag an item | Right trigger; hold while moving the item and release to place it |
| Context / alternate click | Left Grip + Right Trigger (hold grip first) |
| Native Ctrl-click shortcut | Hold Left Trigger, then click with Right Trigger; the game chooses drop, sell or transfer for the current panel |
| Native Shift modifier | Hold Right Grip while using the laser |
| Back / close | Right B, or point and click the panel's close/back button |
| Tabs / page controls | Point and click the native on-screen controls |

Dragging keeps the item held if the ray briefly leaves the panel. Releasing
the trigger ends the drag at the last valid pointer position. Tracking or
desktop focus loss releases the injected mouse button; release the trigger
before starting again. Pressing or releasing Left Grip during a drag will
not convert it into a context-click.

#### Native menu prompts

| Game's displayed input | Quest control | Steam Frame control |
| --- | --- | --- |
| A | Right A | Right A |
| B / back | Right B | Right B |
| X | Left X | Left A |
| Y | Left Y | Left B |
| Left stick / right stick | Left stick / right stick | Left stick / right stick |
| LS / RS click | Left stick click / right stick click | Left stick click / right stick click |
| Inventory: X / drop selected item | Left X; hold if the native prompt requires it | Left A; hold if the native prompt requires it |
| Inventory: hold RS / sort | Press and hold right stick click | Press and hold right stick click |
| LB / RB | Left Grip / Right Grip | Left Grip / Right Grip |
| LT / RT | Left Trigger / Right Trigger | Left Trigger / Right Trigger |
| Menu / Start, where available to the game | Controller menu button | Controller menu button |

Native menu actions use the same buttons in the main menu, tabbed menu, stash,
inventory and merchant panels. **Left primary opens the tabbed menu during
gameplay; inside a native menu it performs the displayed X action.**
**Left Grip + left secondary** stays reserved for VR settings.
Right-stick click does not swap weapons during gameplay. Use the shoulder
gesture or **EQUIPMENT / BELT** wheel for weapon configurations.

Quest controllers do not need a separate D-pad or Xbox controller: use sticks
for native navigation and the laser for the on-screen controls. When using
the laser, Right Trigger is a mouse click and the grips/triggers above provide
its modifiers; once native navigation is selected, they perform LB/RB/LT/RT.
Release buttons, triggers and sticks when opening or closing menus so a held
input cannot also act in the next screen or in gameplay.

The mod's own settings and calibration panels use their VR laser controls:
right laser + trigger to select, Right B to close/cancel. Native gamepad input
does not pass through those panels.

After setup, connect your headset and double-click **Launch-VR-SteamVR.cmd**
or **Launch-VR-VDXR.cmd**. Select an offline character when the game opens.
For ordinary desktop play, use Steam's normal **Play** button. Fully exit
the VR session before switching to desktop play or another headset connection.
You can also add separate **SteamVR** and **Virtual Desktop** entries to your
Steam library using the [shortcut instructions](INSTALL-WINDOWS.md#add-steam-library-shortcuts).

See [Install and play](INSTALL-WINDOWS.md) for setup, controls and removal.

For the complete additions overview, download the
[PDF](https://github.com/bearstonem/d2r-pcvr/blob/main/docs/D2R-PCVR-Additions.pdf) or
[editable Word document](https://github.com/bearstonem/d2r-pcvr/blob/main/docs/D2R-PCVR-Additions.docx).

## Credits

D2R PCVR is built on **[D2R VR](https://github.com/natpoh/diablo2-vr-port) by
[BodyWalkVR](https://bodywalkvr.com/mods/d2r-vr)**, the original VR mod for
Diablo II: Resurrected. Its D2RLoader plugin, first-person view, painted skies
and depth fog are the foundation this project started from, used under its
[MIT license](LICENSE). Thank you to BodyWalkVR for releasing it as open source.
Their original documentation is kept in [UPSTREAM.md](https://github.com/bearstonem/d2r-pcvr/blob/main/UPSTREAM.md). For their
own release, which runs through BodyWalk and FlatVR, visit the
[D2R VR page on BodyWalkVR](https://bodywalkvr.com/mods/d2r-vr).

D2R PCVR adds native OpenXR support for SteamVR and Virtual Desktop (VDXR),
motion combat, the physical bow and potions, VR menus, foveated rendering and
the guided Windows installer. Licenses for all third-party components are in
[THIRD_PARTY.md](THIRD_PARTY.md).

This project is not affiliated with or endorsed by Blizzard Entertainment.
