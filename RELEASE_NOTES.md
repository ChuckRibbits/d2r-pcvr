# D2R VR Unleashed v0.3.20 - Windows preview

This update focuses on hand-aimed spells, physical combat, wrist navigation,
foveated rendering and clearer recording and launcher options.

**Draft recording candidate. Public release assets are not published yet.**

## Gameplay and combat

- **Defense that reduces incoming damage - contributed by ChuckRibbits:**
  In Unleashed, monster attacks no longer disappear because of a failed hit
  roll. Instead, the native chance to hit your character scales the damage of
  an attack that connects. Defense and character level still matter, while
  dodging, shield blocking and parrying remain useful. Classic retains its
  native hit rolls.
- **Timed parries and clearer feedback - contributed by ChuckRibbits:**
  Shield parries use a 450 ms window and must overlap the enemy's attack
  wind-up. An unused parry sweep produces a short, soft vibration on the shield
  hand. A successful parry still opens the 1.5-second riposte opportunity.
- **Physical Assassin dual claws:** Equip two claws and swing or thrust with
  either hand, in both Unleashed and Classic. Active skills retain their normal
  button assignments.
- **Hand-aimed Bone Spear and Holy Bolt:** Assign either casting hand's trigger
  or shortcut and hold to repeat while walking or turning. Planar spell aiming
  Off allows up/down controller aim. Projectile aim assist supports Off, Light
  launch correction, or Strong launch correction and gentle homing. Holy Bolt
  retains native damage against undead/demons and native friendly healing.
- **Controlled bone placement:** Bone Wall and Bone Prison take their ground
  target from the casting controller. Point downward to choose a location;
  level or upward aim places bones six cells ahead, subject to native placement
  rules.

## Controls and interface

- **Top-down wrist map - contributed by beadlebytes:** Choose Isometric or
  Top-down under **VR Settings > HUD > Map view**. Enable **Rotate map** to keep
  your facing direction at the top when using Top-down.
- **Clearer skill selection - contributed by beadlebytes:** More skills show
  their proper names in the picker, making assignments easier to recognize.
- **One-eye desktop recording - contributed by beadlebytes:** The monitor can
  show the left eye for cleaner recordings and streams while the headset keeps
  its stereo view. **VR Settings > Video > Desktop shows one eye** is On by
  default and can be disabled.
- **Shared launcher integration:** Setup registers Unleashed's own name, icon,
  version, support links, logs and update information with the D2R VR Launcher.
  The standalone installer and launch options remain available. Thanks to
  **Sergious** for the shared launcher and integration foundation.

## Visuals and stability

- **Working foveation and adjustable strength:** Fixed and eye-tracked
  foveation now apply to the visible world shading pass. Under
  **VR Settings > Video > Foveation strength**, choose Low, Medium, High or
  Maximum. Higher strengths shade more of the periphery coarsely; lower
  strengths favor clarity. The frame-rate benefit depends on the GPU workload
  and CPU limits.
- **Eye-tracked rendering:** Supported runtime gaze input moves the detailed
  region with your view. Steam Frame through SteamVR requires **Share eye
  tracking data** to be enabled in SteamVR. Without usable gaze, Eye tracked
  retains full shading quality.
- **Clearer Kurast sewer entrances:** Entrance and stairwell surfaces render
  from both sides, dark tunnel backdrops cover the exposed openings, and a
  nearby entry prompt makes the Right A interaction easier to find.

## Updating

When published, use **D2R-PCVR-Setup-0.3.20.exe** for graphical installation or
**d2r-pcvr-windows.zip** for portable setup. Existing users should choose
**Update / repair and keep calibration and video settings**.

Existing settings, calibration and saves are preserved. The recording candidate
is installed separately as **pcvr-dev**; it does not replace the normal mod.

## Verification and known issues

The recording candidate passed **110 native checks** and **34 Python checks**
covering release-note generation, version identity and installer staging.
The in-headset page was rendered and visually inspected for readable feature
lines, version, author, project link and credits.

The gameplay changes were checked during development in the offline developer
environment. Foveation was checked on Quest 3 through Virtual Desktop and
Steam Frame through SteamVR, including usable eye-tracking input on Steam Frame.
The corrected Holy Bolt candidate was verified before merging. The rebuilt
v0.3.20 recording candidate still needs its own in-headset release-page check.

Final release packaging and acceptance checks remain pending. Spell collision
and ground placement retain the existing launch-floor approximation; assistance
does not guarantee a hit. Battle.net remains experimental. The trial forearm /
shield-position changes and Sorceress elemental-hands prototype are excluded.

## Special thanks

Thanks to **ChuckRibbits** for combat and defense improvements, **beadlebytes**
for the map, skill picker and desktop view contributions, **Sergious** for the
launcher foundation, and everyone testing and reporting issues!

D2R VR Unleashed is a mod by **monkeylashes**, based on the original D2R VR work
by **Sergious / BodyWalkVR**, with contributions from the community.

## Downloads and links

- [Release page when published](https://github.com/bearstonem/d2r-pcvr/releases/tag/v0.3.20-windows-preview)
- [Installation instructions](https://github.com/bearstonem/d2r-pcvr/blob/main/INSTALL-WINDOWS.md)
- [Controls](https://github.com/bearstonem/d2r-pcvr/blob/main/CONTROLS.md)
- [Changes since v0.3.19](https://github.com/bearstonem/d2r-pcvr/compare/v0.3.19-windows-preview...main)
- [Project on GitHub](https://github.com/bearstonem/d2r-pcvr)

<!-- PCVR_IN_HEADSET_NOTES
release: v0.3.20-windows-preview
credits: ChuckRibbits; beadlebytes; Sergious / BodyWalkVR.
- Unleashed: monster hits connect; Defense reduces damage instead of rolling misses.
- Parries: 450 ms timing window and shield-hand feedback for unused sweeps.
- Assassin dual claws: swing or thrust with either hand in both melee modes.
- Bone Spear: cast from either hand while moving, with free aim and optional homing.
- Bone Wall / Prison: aim with your casting controller to choose the ground location.
- Holy Bolt: full hand aiming and optional homing; native damage and healing rules.
- Wrist map: Isometric or Top-down view, with optional rotation under HUD.
- Skill picker: proper names for more skills.
- Desktop recording: one-eye monitor view, adjustable under Video.
- Kurast sewers: fuller entrances, dark tunnel backdrops and visible entry prompts.
- Foveation: fixed and eye-tracked modes now shade the visible world.
- Video: Foveation strength - Low, Medium, High or Maximum.
- Shared launcher: Unleashed's name, icon, version and support links.
PCVR_IN_HEADSET_NOTES -->
