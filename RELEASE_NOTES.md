# D2R PCVR v0.3.13 - Windows preview

This release brings together the controls, spellcasting, navigation, rendering,
menu and Quest fixes developed since v0.3.12.

Download **D2R-PCVR-Setup-0.3.13.exe** for graphical installation, or
**d2r-pcvr-windows.zip** for portable setup. Existing users should choose
**Update / repair and keep calibration and video settings**, keeping their
existing helper folders and setup backups.

You need your own purchased Diablo II: Resurrected: Steam 3.3.93787 or
Battle.net 3.3.93847 (experimental). Use local offline characters only.
The installer includes no Blizzard game assets; icons and potion models are
read locally from your installed game. SteamVR and Virtual Desktop/VDXR are
supported connection options. The EXE is unsigned; checksums accompany both downloads.

## Controls and combat

- **Two independent skill wheels:** click either thumbstick to toggle that
  hand's wheel, then use the other hand's laser and trigger to assign skills.
  Each hand has trigger, two face-button, and two grip-plus-face-button slots.
- **Controller-aware labels:** automatic detection and a manual Headset /
  controllers dropdown for Quest/Touch, Index, Steam Frame, or Generic labels.
- **Reliable menu access:** left grip + left stick click opens the native
  inventory/character/skills menu, including controllers whose Menu button is
  reserved by the runtime. Either wheel also offers INVENTORY.
- **Weapon skills and potions:** compatible trigger skills apply to physical
  weapon gestures; weapon-mounted skill indicators are optional and off by
  default. Grabbed potion bottles sit upright with corrected drinking alignment.
- **Independent Sorceress casting:** Fire Bolt, Fireball, Ice Bolt, Ice Blast
  and Charged Bolt can fire from both hands together, with independent aiming
  and repeat timing, native mana costs and Faster Cast Rate. Walking and turning
  remain available during these supported casts.
- **Continuous aiming:** spell direction follows the casting hand/weapon through
  movement, physical rotation and snap turns. Supported straight projectiles
  launch from the hand/weapon; Ice Bolt has corrected launch height. Charged Bolt
  stays on the ground and retains its native spread and wandering.
- **Planar spell aiming:** on by default; switch it off for vertical aiming of
  supported straight spells. Horizontal aiming remains continuous in either mode.
- **Optional projectile aim assist:** Off by default, with Light launch assistance
  and Strong launch assistance plus gentle target-locked homing for straight
  hand spells. Charged Bolt uses launch assistance only. Bows, thrown weapons
  and unsupported native spells are unchanged. Strong is not a guaranteed hit.
- **Cast feedback:** restore native casting-start sounds and character refusal
  voices. Town-restricted spells use the town warning, while insufficient mana
  outside town uses the mana warning, retaining the game's voice cooldown.
- **Turning choice:** optional smooth turning with independent speed and snap-angle
  settings. New profiles default to 30-degree snap turns.

## Navigation and HUD

- **Minimap and compass together:** an optional left-hand navigation display,
  alongside the map in both wheels. Select Wrist, Palm, Either or Off as one unit.
  The compass shows bearings, not quest or destination markers.
- **Independent vitals placement:** health/mana/XP can use Wrist, Palm, Either or
  Off. Palm placement uses a deliberate palm-up gesture; automatic low-resource
  alerts are a separate option and off by default.
- **Compact hand maps:** temporarily use a corner-map layout for VR capture even
  when the game's full-screen automap is selected, restoring native layout afterward.
- **Flat HUD removed:** suppress the original bottom toolbar and its menu-transition
  flashes in VR. Guard hidden experience widgets against the reported casting/
  level-up crash. Desktop HUD behavior is preserved.
- **Monster bars:** ordinary monster bars are hidden by default, with a toggle;
  bosses, champions and other elite bars retain their native behavior.

## Resolution and menus

- **Monitor-independent VR resolution:** choose Desktop, Headset, or fixed presets
  up to 4096 x 4096 in VR Settings > Video. A low-resolution desktop monitor no
  longer caps the VR render source. Higher resolutions increase GPU load.
- **Graphics Apply recovery:** reapply the VR resolution automatically after
  changes in the game's video options reset its rendering surfaces.
- **Square-render corrections:** fix fog/depth behavior and sharp rectangular
  lighting patches at square VR resolutions; added fog and sky remain available.
- **Comfortable menus:** eye-level inventory, merchant and main menus with
  Close / Medium / Far distance presets. Medium is 30 cm farther than the early
  private-build placement. Startup placement waits for tracked headset poses.
- **Native UI and laser alignment:** maintain a widescreen layout within the VR
  render source, correct event scaling and cursor bounds, and restore access to
  bottom-row Play/Create Character buttons on Quest at custom resolutions.
- **NPC and waypoint panels:** lower and enlarge dialogue choices, correct their
  crop and merchant placement, retain the complete dialogue canvas on Touch
  controllers, and give both waypoint eyes matching black backgrounds.
- **Item tooltips:** immediate laser hover and eligible equipped-item comparisons,
  with Instant / Short (300 ms) / Original timing options and more consistent
  item-grid targeting.

## Getting started

1. Install or update, reopen Steam and connect your headset.
2. Use the SteamVR shortcut for Steam Link/SteamVR, or the VDXR shortcut for
   Virtual Desktop. Choosing VDXR in Streamer alone does not change a SteamVR launch.
3. Click either stick to open its wheel. Use the opposite hand to assign spells.
4. Open VR SETTINGS and Apply your changes: controller labels under VR; aiming
   under Controls; hand displays/menu distance/tooltips under HUD; resolution
   under Video.
5. Use left grip + left stick click whenever you need the native game menu.

See [installation instructions](INSTALL-WINDOWS.md) and [controls](CONTROLS.md).

## Validation and limitations

The release candidate passed all 57 native checks and 114 Python checks covering
setup, repair/removal, launchers, asset preparation and package staging. The
compiled installer lifecycle fixture passed installation, registration and
removal using an isolated stub backend. These fixtures do not modify a real
game installation. Headset feedback on Steam
D2R with Quest 3/Virtual Desktop/VDXR confirmed menu access and bottom-row pointer
selection, NPC dialogue, and correct town/mana voice feedback. Earlier offline
sessions confirmed dual casting, planar aim, projectile-height fixes, square
resolution lighting, and added fog/sky. These are reported gameplay checks,
not a claim that every setting, class, runtime or area has been exhaustively tested.

Strong homing still needs wider headset testing. Socketed-item tooltip overlap,
long sessions, every menu/resolution combination, and repeated graphics changes
remain useful regression scenarios. The new Quest 3 Steam Link session has not
been reported complete. Battle.net support remains experimental; this release
has no new Battle.net or Quest 2 gameplay validation.

Dual casting and movement freedom apply to the supported Sorceress projectiles;
other skills keep their native restrictions. Two hands intentionally allow more
throughput than one. Planar assist cannot hit a body entirely above/below its
launch plane, and terrain handling retains the launch-floor approximation.

Existing installer backup discovery, verified downloads, repair and removal are
retained. The released phase 1 performance improvements are retained. The closed
experimental performance phase 2 PR (#24) and unrelated local rendering
prototypes are not included. Rendering still uses two game draws per stereo pair.
