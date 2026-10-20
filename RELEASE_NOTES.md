# D2R VR Unleashed v0.3.21 - Windows preview

This update focuses on smoother VR rendering, shield combat, directional sound
and easier installation with the shared launcher.

## Gameplay and combat

- **More demanding shield combat (Unleashed) - contributed by ChuckRibbits:** Champions,
  uniques and act bosses build guard strain faster than ordinary monsters.
  A broken guard allows chip damage through, with native resistances and damage
  reduction still applying. Physical blocks can wear down your shield, and a
  bone-snap sound signals a guard break. Act bosses can be blocked but not parried.
- **Parries that meet the visible blow - contributed by ChuckRibbits:** Parry
  timing now follows the enemy's downstroke and resolves at contact. A short
  grace period after a blocked hit helps account for the difference between
  the game's damage timing and the animation. Successful parries retain the
  riposte opportunity; they do not undo damage already taken.
- **Guard recovery between attacks - contributed by ChuckRibbits:** Strain
  fades after attacks stop, even while your shield stays raised. Lowering the
  shield starts recovery immediately. Broken guards recover once fully cleared.
- **Low-life heartbeat - contributed by ChuckRibbits:** The heartbeat follows
  your health in both melee modes, growing faster and louder as life falls.
  **VR Settings > Combat > Heartbeat starts at** offers Off or 10-90%, default
  60%. Healing quiets it; wiping the blood visor clean hushes it until another hit.

## Controls and interface

- **Optional enemy outlines - contributed by beadlebytes:** Under
  **VR Settings > Aim**, enable a subtle outline around the hostile monster
  you are pointing at. Choose strength, a static color or colors by range.
  **Off by default.** Town residents, including camp cows, are excluded.
  The outline is a visual guide and does not change attack aiming.
- **Clearer settings help:** Hover help explains individual rendering and other
  named choices. Video warnings identify game VSync, anti-aliasing settings and
  disabled deferred GPU waits that can interfere with the recommended setup.
  A separate warning explains where to turn off Windows Mono audio when it
  would prevent directional sound.

## Visuals and stability

- **New stereo rendering modes:** **VR Settings > Video > Stereo rendering**
  offers **By turns** (the new default), **Both eyes** and **Replay**. By turns
  draws one eye each frame and reprojects the other using head motion and scene
  depth, reducing the game's CPU rendering work. Both eyes retains the previous
  rendering method. Replay reuses one game draw for both eyes once an area
  settles, returning to two passes when needed.
- **Smoother hand displays:** Wrist and wheel image updates reuse upload
  resources, reducing refresh stalls. By turns keeps belt potions and game
  labels out of the world reprojection so they remain more stable in the headset.
- **Rendering defaults for VR:** Render resolution defaults to the headset's
  recommendation. Fresh installs enable deferred GPU waits. Existing installs
  retain their setting; if it is disabled, **VR Settings > Video > Defer GPU
  wait**, then **Apply**, enables it without restarting.
- **Head-relative directional sound - contributed by ChuckRibbits:** Native
  3D sounds such as nearby monsters, NPCs and streams pan as you turn your head
  or use stick turning. Music, UI and other native 2D sounds remain centered.

## Updating

Download **D2R-PCVR-Setup-0.3.21.exe** for graphical installation, or
**d2r-pcvr-windows.zip** for portable setup. Existing users should choose
**Update / repair and keep calibration and video settings**.

The Windows installer now includes **D2R VR Launcher 0.3.5**, with thanks to
**Sergious**. It installs or updates the launcher when needed and preserves a
newer installed version. Choose Unleashed and an OpenXR runtime in the launcher;
Authentic remains a separate optional installation. Direct Steam shortcuts and
portable setup remain available, and a launcher installation failure does not
prevent installing Unleashed.

Close the game, Steam and the shared launcher before updating. Existing settings,
calibration and saves are preserved. Stereo rendering starts on **By turns**;
choose **Both eyes** to return to the previous rendering method.

## Verification and known issues

The production build passed **114 native checks** and **217 Python checks**.
One additional Python symlink check was skipped because this Windows session
cannot create symbolic links. The compiled installer lifecycle passed with an
isolated stub backend, including registration and failed-removal recovery.
The in-headset release page was rendered and visually checked for the version,
author, project link, credits and readable feature lines.

Launcher 0.3.5 source passed **22 checks**, including its GUI and install view.
The compiled Unleashed installer was also checked with an isolated launcher
fixture for fresh installation, upgrade, equal/newer-version preservation,
nonblocking launcher failure and leaving the launcher after mod removal.

Feature testing during development used Steam Frame through SteamVR on Windows,
Steam D2R 3.3.93787 and offline developer characters. Rendering, revised shield
combat, directional audio and the optional outline were checked in that
environment. The contributor also checked the outline on Quest 2 through SteamVR
with the Battle.net edition. This is feature-level testing, not a fresh headset
pass of the final release package.

By turns draws each eye at half the headset submission rate. Transparency,
fast hand motion and moving labels can still show reprojection artifacts.
Use Both eyes if you prefer its image consistency. Replay can pause frequently
while areas stream, and DLSS history can smear between eyes; FXAA is recommended.
Performance varies by hardware, runtime and scene. The new rendering paths have
not been verified on Quest 3 through Virtual Desktop or under Linux/Proton.
Battle.net remains experimental. Multiplayer and the Sorceress elemental-hands
prototype are excluded.

## Special thanks

Thanks to **ChuckRibbits** for shield combat, the low-life heartbeat and
directional audio, **beadlebytes** for enemy outlines, **Sergious** for the
shared launcher and shared rendering work, and everyone testing and reporting
issues!

D2R VR Unleashed is a mod by **monkeylashes**, with contributions from the community.

## Downloads and links

- [Windows installer, portable ZIP and checksums](https://github.com/bearstonem/d2r-pcvr/releases/tag/v0.3.21-windows-preview)
- [Installation instructions](https://github.com/bearstonem/d2r-pcvr/blob/v0.3.21-windows-preview/INSTALL-WINDOWS.md)
- [Controls](https://github.com/bearstonem/d2r-pcvr/blob/v0.3.21-windows-preview/CONTROLS.md)
- [Full source comparison](https://github.com/bearstonem/d2r-pcvr/compare/v0.3.20-windows-preview...v0.3.21-windows-preview)
- [Project on GitHub](https://github.com/bearstonem/d2r-pcvr)
- [Discord](https://discord.gg/DmbFfJAnnm)
- [Support](https://ko-fi.com/bearstonem)

<!-- PCVR_IN_HEADSET_NOTES
release: v0.3.21-windows-preview
credits: ChuckRibbits; beadlebytes; Sergious.
- Stereo rendering: By turns (default), Both eyes or Replay under Video.
- By turns: depth and head-motion reprojection, with steadier potions and game labels.
- Faster wrist and wheel refreshes reduce display update stalls.
- VR resolution defaults to Headset; fresh installs enable deferred GPU waits.
- Video: Defer GPU wait button and warnings for game VSync and anti-aliasing.
- Shield combat: stronger enemies strain guards faster; breaks allow chip damage.
- Shields wear on physical blocks; a bone-snap sound signals a guard break.
- Parries follow the visible downstroke; act bosses can be blocked, not parried.
- Guard strain recovers when attacks stop, even while the shield stays raised.
- Low-life heartbeat: choose its starting health percentage under Combat.
- Directional 3D sound follows head and stick turns; Windows Mono audio warning.
- Aim: optional subtle hostile-monster outlines, strength and colors; default Off.
- Settings help explains individual named choices.
- Windows Setup includes the shared D2R VR Launcher 0.3.5.
PCVR_IN_HEADSET_NOTES -->
