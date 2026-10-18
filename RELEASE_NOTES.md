# D2R VR Unleashed v0.3.17 - Windows preview

This update focuses on physical bow skills, helping your mercenary, world
visuals, Act III stability and a cleaner installation. D2R PCVR is now presented
as **D2R VR Unleashed**; the repository and download filenames keep the PCVR name.

Download **D2R-PCVR-Setup-0.3.17.exe** for graphical installation, or
**d2r-pcvr-windows.zip** for portable setup. Existing users should choose
**Update / repair and keep calibration and video settings**. Keep your existing
helper folders and setup backups until the update finishes. Full install applies
fresh defaults; use Update / repair to retain your settings and calibration.

You need your own purchased Diablo II: Resurrected: Steam 3.3.93787 or
Battle.net 3.3.93847 (experimental). Use local offline characters only.
SteamVR and Virtual Desktop/VDXR remain supported. No Blizzard game assets are
included; icons and potion models are read locally from your installed game.
The installer is unsigned; SHA-256 checksums accompany both downloads.

## Bows and arrow skills

- **Physical skill-arrow quiver - contributed by aSuperGod:** with a bow
  equipped, press a face button assigned to Magic, Fire, Cold, Exploding, Ice,
  Immolation or Freezing Arrow to load that skill. Draw and release normally to
  fire it using your physical aim. Press the same button again for basic arrows,
  or another arrow skill's button to switch, including while drawing.
- **Native skill costs and effects:** each shot uses the skill's own mana cost,
  damage and effects. If the game refuses the skill, such as for insufficient
  mana, the drawn arrow falls back to a basic attack. Multiple Shot, Guided
  Arrow and Strafe retain their existing behavior.
- **See your selected arrow:** the nocked arrow displays the selected skill's
  model and particles. Model changes now wait for native cleanup before replacing
  the previous arrow, addressing the preview-swap lifecycle problem. A brief
  basic-arrow fallback can appear while the new model loads.
- **Impact-effect placement:** skill-arrow bursts use the swept contact point
  and compensate for the native effect's fixed height offset, addressing effects
  floating above low ground shots. Native splash spacing is retained; persistent
  ground fire and moving freeze ejecta retain native placement.

The quiver and selected-arrow preview default On. Advanced overrides remain
available through `[bow] quiver=0` and `quiver_preview=0`; explicit saved choices
are preserved.

## Mercenary interactions

- **Toss potions to your mercenary - contributed by ChuckRibbits:** grab a belt
  bottle with Grip, swing your hand and release it so it lands near your
  mercenary's feet. Health, Rejuvenation, Antidote and Thawing potions are
  supported. A successful toss consumes the potion and applies its native
  effect to the mercenary; a miss keeps the potion.
- **Low-health warning:** your living mercenary gains a red tint below 50%
  health, helping you spot when it needs a potion. The warning takes priority
  over poison and cold coloration and clears on recovery or death.

## World visuals and Act III

- **Roofs stay visible in first person:** suppress the game's environmental
  cutaway requests so entering a building no longer deliberately hides its roof.
  This uses the shared cutaway mechanism across areas; it does not create missing
  ceilings or repair model undersides. Verified in Kurast Docks.
- **Hanging-fixture motion:** partial-ragdoll props advance their physics once
  per stereo pair, while both eyes receive the resulting pose. This targets
  erratic hanging-lantern motion during the tiny second-eye time step.
- **More room for Act III rendering - based on Sergious / BodyWalkVR's fix:**
  guarded startup checks expand the renderer pool from 12 to 64 MiB and the
  entity pool from 70 to 280 MiB, targeting the reported renderer-allocation
  crashes in the jungle. Unexpected allocator layouts are left unchanged.
  This adds memory capacity; it is not an FPS optimization or a room-unloading fix.

## Installation and updates

- **Separate mod folder:** the plugin, settings, OpenXR loader, ReShade files
  and generated caches now live under `mods/D2R-PCVR`. Updated shortcuts select
  this mod through D2RLoader. This prepares separate installations for Unleashed
  and Authentic; the shared launcher is not included in this release.
- **Own plugin identity and filenames - built on Sergious' contribution:**
  the plugin now reports its own name, ID and release version, with upstream
  credits retained. Its files are `d2rl-pcvr.dll` and `d2r_pcvr.ini`.
- **Settings migration:** Update / repair recognizes recorded older PCVR
  installations, migrates their settings/calibration and owned ReShade files,
  and retains existing mod-folder settings. Normal offline characters remain
  available. Other mods' files and shared loader dependencies are protected
  during repair, rollback and removal.
- **Clearer setup guidance:** confirmed Windows Security blocks now show
  concise recovery steps. Setup never changes antivirus settings and still
  verifies dependency checksums. A read-only HDR check recommends turning HDR
  off when it is enabled, to avoid reported visual problems; setup leaves the
  display setting under your control.

Use the updated installer and launch shortcuts for this migration. Copying only
the renamed DLL into an older setup does not update its layout or launch path.
Existing global VR plugins can block installation to prevent both VR layers
loading together; setup identifies the conflict before changing game files.

A huge thank-you to **aSuperGod** for the bow quiver, **ChuckRibbits** for the
mercenary potion interactions, and **Sergious / BodyWalkVR** for the identity,
mod-folder groundwork and pool fix. Thanks to everyone reporting issues and
testing these changes in the headset!

## Validation and remaining limits

The release build passed **83/83 native checks** and **170/170 Python checks**
covering installation, migration, repair/removal, launchers, HDR detection,
asset preparation and package staging. The compiled installer lifecycle fixture
passed installation, registration, failed-removal recovery and successful
removal with an isolated stub backend. No game files are used as test fixtures.

The owner verified full installation and subsequent Update / repair, retained
settings/calibration/characters, Kurast roof retention, mercenary potion tossing,
quiver firing and the selected nocked appearance during development. Act III
loaded and worked with both expanded pools active, but a long crash-reproduction
session was not performed. These confirmations do not cover every headset,
runtime, skill or area.

The newest arrow-impact correction and hanging-fixture fix have not received
separate headset confirmations. The final release artifact has not had a fresh
complete headset pass. Longer Act III stability, VDXR with the new mod layout,
switching alongside an independently installed Authentic mod, real uninstall,
and the HDR-enabled wizard path still need broader testing. Fixtures cover
migration and removal but do not replace those real-installation checks.

Battle.net remains experimental. Airborne-enemy targeting is unchanged.
Projectile terrain collision still uses the existing launch-floor approximation.
The DEV menu, test equipment/travel helpers and unmerged elemental-hands
prototype are excluded from the release.

See [installation instructions](INSTALL-WINDOWS.md) and [controls](CONTROLS.md).

[Full source comparison](https://github.com/bearstonem/d2r-pcvr/compare/v0.3.16-windows-preview...v0.3.17-windows-preview)

<!-- PCVR_IN_HEADSET_NOTES
release: v0.3.17-windows-preview
credits: aSuperGod (quiver), ChuckRibbits (mercenary), Sergious / BodyWalkVR.
- D2R PCVR is now named D2R VR Unleashed.
- Bow quiver: select an arrow skill with its assigned face button.
- Skill arrows use native mana costs, damage and effects.
- The nocked arrow shows the selected skill's model and particles.
- Arrow impact bursts follow the contact point on low shots.
- Toss health, rejuvenation, antidote or thawing potions to your mercenary.
- Mercenaries turn red below 50% life; the tint clears on recovery.
- Building roofs remain visible when entering in first person.
- Hanging fixtures advance physics once per stereo pair.
- Larger renderer and entity pools target Act III allocation crashes.
- Unleashed installs separately under mods/D2R-PCVR.
- Own plugin identity: d2rl-pcvr.dll and d2r_pcvr.ini.
- Update / repair preserves settings, calibration and normal saves.
- Setup explains Windows Security blocks and checks for enabled HDR.
PCVR_IN_HEADSET_NOTES -->
