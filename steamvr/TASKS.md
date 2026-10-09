# D2R Steam Frame PCVR tasks

Updated 2026-10-08. Target: first-person D2R on this Linux PC through Proton,
SteamVR streaming to Steam Frame. Test with offline character **Nekro** only.
The DevilutionX standalone/tabletop project remains parked separately.

Unchecked tasks remain open, including implemented changes awaiting headset
verification. Build checks and automated tests do not establish visual correctness.

User verified the changes so far in the 18:20 build, then reported inventory
closing on laser clicks and no visible belt. Individual regression scenarios
below remain explicit so they are not lost after that general check.

## 1. Stability and calibration — next headset pass

- [ ] Verify the background scan freeze fixes in a longer play session and in
  a dungeon. Checked memory reads fixed the hero-facing scan crash; the latest
  build also snapshots the fog scan and checks writes. Proton memory tests pass.
- [ ] Verify the corpse at the spawn location no longer drives or duplicates
  the VR body. The latest build filters player skeletons by proximity and original
  posed head height. Selection tests pass; this is a heuristic, not live-unit-ID
  binding. Check death, respawn, crouching and attack animations; retain the
  corpse's normal game behavior and items.
- [ ] Verify automatic recenter on every saved-game load after moving during
  launch. The latest build calibrates on the first focused, tracked world frame
  and recenters facing. Opening inventory/dialogue must not recalibrate.
- [ ] Verify height reset: hold **left grip + click left stick**. Preserve map
  on plain stick click, horizontal room movement, heading and physical eye
  separation. Input/math tests pass. **F11** performs full position/facing reset.
- [ ] Verify Exit Diablo II from the headset returns Steam to **Play**, without
  a separate PC-side Stop. The VR-only launch monitor cleans this launch's
  lingering Proton helpers after the game exits. Three process tests pass;
  leave the Steam client, SteamVR and unrelated programs running.

## 2. Menus and HUD

- [ ] Verify floating creature names/health converge at the creature in both
  eyes, including close/distant creatures, head movement, bosses and ground items.
  User reports doubled labels on normal monsters (name and health in one label).
  The new unit-label projection hook retains native world anchors and uses the
  exact current-eye scene matrices, avoiding inline flat-frustum rebuilding.
  Stereo anchor/ray convergence checks pass; headset verification pending.
  The first matrix-only prototype still doubled in the user's test (native
  projected positions differed by less than a pixel). The next build combines
  the original foot X/Z and head height in world space before projecting once,
  replacing native projected-foot-X/projected-head-Y mixing. Verify this new
  anchor with head pitch/roll; the user tested it and **labels still double**.
  Stop treating projection math tests as visual proof. Next inspect single-eye
  label draws, cached UI layout, rectangle placement and eye/frame pairing.

- [ ] Verify inventory stays open when selecting/dragging items with the laser.
  User found that the first trigger click closed it. Logs show controller-layout
  inventory opening then closing immediately on laser mouse down, with neutral
  mirrored pad input. Native inventory now opens with the mouse-layout `I` key;
  suppress stale gamepad actions in menus and consume a menu trigger until release
  so closing a panel cannot attack. The 18:27 virtual-key toggle failed to open
  inventory in the user's test; logs confirmed button detection without a panel
  event. Correct injection to scan-code down, hold across frames, then key up;
  release on focus/tracking loss. Corrected build installed and launched at 18:34;
  live logs confirmed initial opening, but the user found that Left X stopped
  opening menus after switching to other panels and closing them. Replace the
  keyboard toggle with native UI-thread PanelManager actions: close open menu
  panels, otherwise open inventory. Keep HUD/map alive. Test repeated inventory
  -> other shortcut panels -> Left X close -> Left X reopen cycles and item dragging.
  The 18:45 build's live logs confirm inventory reopened after closing inventory
  and character panels; visual confirmation and repeated cycles remain pending.
- [ ] Verify NPC conversation panels are stable and identical for both eyes.
  The user reported double vision and choppiness. The latest build disables
  eye alternation and VR camera/projection for panel frames; headset check pending.
- [ ] Verify the implemented right-controller cyan laser in title, dialogue,
  inventory, character, skill and pause menus. Right trigger clicks/drags;
  right B backs out. Test item dragging, tooltips, focus loss and panel placement.
  Menu intersection math checks pass; the user has used dialogue selection.
- [ ] Design health/mana and belt presentation for VR. User did not see the belt
  in the latest build and prefers avoiding a constantly floating panel. Prioritize
  an on-demand wrist/waist belt and quick potion controls; compare compact wrist
  health/mana, situational feedback and an optional persistent display. Use original
  icons and keep potion counts readable. Dedicated VR belt is not implemented.
- [ ] Extract the real UI texture with correct alpha, rather than cropping a
  backbuffer containing scenery. Upstream HUD sharing exists; native OpenXR
  transport does not consume it yet. Keep menus/text at full quality.
- [ ] Provide pointer selection and quick use of belt potions without opening
  inventory; verify tooltips, potion types, hotkeys and feedback.

## 3. Weapon and spell interaction

- [ ] Make world interaction obvious and precise: right-hand aim ray, highlight
  and readable name/action for the actual selected target, then **Right A** to
  interact/loot. Current Right A uses the game's native interaction target;
  that is not yet a verified hand-ray hit-selection system. Support items, doors,
  NPCs and corpses; prevent selecting through walls or distant scenery, add target
  stability and clear range feedback, and suppress targeting while menus are open.
- [ ] Route Right A exclusively through the semantic interaction action: its
  current raw XInput A mirror can bypass the no-target guard and attack on a miss.
  Keep trigger attack separate; test that pointing at empty ground and pressing
  loot cannot attack, walk or choose a different nearby target.
- [ ] Test recovery of Nekro's own equipment-bearing corpse in camp. User sees
  an existing corpse and an unequipped live character. Keep native corpse recovery
  and inventory-capacity rules, with visible success/failure feedback. Do not remove
  corpses or modify save/inventory data to work around this. The cause of that
  corpse was not established; the body-rendering filter is separate from looting.
- [ ] Document and verify current controller attack, interaction, weapon swap,
  selected skill and aim bindings. Current trigger path invokes game actions;
  tracked weapon presentation does not establish gesture combat support.
- [ ] Add a controller radial selector for weapons and assigned spells/skills,
  with clear equipped/selected feedback and comfortable casting controls.
- [ ] Add optional melee swing detection: intentional swing triggers the game's
  native attack, with thresholds/debounce and trigger fallback. Respect weapon
  reach, game attack speed, hit rules and cooldowns; no attacks from menu gestures.
- [ ] Add optional two-hand bow mechanics: nock/draw/release gesture and controller
  aim trigger the native shot. Preserve ammunition, native damage and attack timing.
- [ ] Define aimed spell casting, self-cast/buff and summon controls separately;
  preserve mana costs/cooldowns and an accessible button alternative.

## 4. Performance and eye-tracked foveation

- [ ] Investigate movement micro-freezes/skips, now the next priority after the
  menu reopening fix. Separate game/asset loading, pipeline compilation, mod
  memory scans, rendering and OpenXR/copy waits. New logs break down >80 ms hitches,
  time native pipeline creation and exclude menu gaps from gameplay statistics.
- [ ] Compare walking after disabling Proton exception/unwind trace logging.
  The 18:45 session recorded pauses up to 728 ms, largely before stereo rendering;
  none of the captured hitches recorded pipeline creation. Its Proton log grew
  to 31 MB in about three minutes, peaking above 38,000 trace lines per second.
  The launcher now retains warnings/errors and game-side timing without those
  trace channels. This is a suspected source of overhead, awaiting comparison.
  Global Wine warnings also produced routine file/debug-string messages; the
  next profile keeps Wine errors and native VKD3D warnings only. The first quieter
  walking test still recorded 30 hitches, up to 1,313 ms, with about 95% of recorded
  hitch time before stereo drawing and no tracked pipeline creation in those
  hitch samples. Continue investigating game loading and CPU-side stalls.
- [x] Verify the buffered logger removes the measured main-thread filesystem
  waits while walking/turning. Live Linux samples showed `fsync()` on the mod's
  `d2r-vr-vrcam.log` during journal/page waits spanning hundreds of milliseconds;
  sampled long pauses had no physical disk reads. Runtime logging now uses a
  bounded queue and worker batches in `d2r-vr-frame.log`, with no forced flushes.
  Concurrent producers preserve their records/order and shutdown drains them
  in the Proton test. User confirmed **hitches improved, labels still doubled**.
  A subsequent 599-sample trace saw no journal/page waits or blocked log paths.
  Typical 10-second timing windows had longest pairs of 38–60 ms; one 475 ms
  gameplay gap remained. Continue investigating residual hitches separately.
- [ ] Compare the same walking route twice and after another launch without
  clearing caches. Shader compilation is a candidate, not an established cause.
  Latest Proton log confirms an active shader cache with 1,820 existing entries
  and 8 merged additions. Pipeline creation callbacks do not capture every form
  of lazy compilation or driver work.
- [ ] Measure retaining native height fog (`game_height_fog=1`) as the new PCVR
  preset. This disables full-memory fog scans scheduled 1/4/12 seconds after
  area changes. Atmosphere may differ; compare hitches and decide whether a
  bounded replacement is worthwhile. Keep scene resolution/other quality fixed.
- [ ] Measure CPU, GPU scene draws, effects, copies and compositor timing in the
  same camp/dungeon scenes. Current pipeline renders two scenes per game update.
  Observed pair rates after the current 80% preset were about 31–35 Hz; this is
  not a controlled benchmark or the headset display refresh rate.
- [ ] Validate the existing 80% scene-resolution/medium AO, atmospherics and
  shadows preset and per-eye copy command lists with a single pair completion
  wait. Preserve textures; compare frame time and visual quality at fixed settings.
- [x] Query **actual PC-streaming** OpenXR capabilities through WineOpenXR and
  D3D12 VRS through vkd3d-proton. Launch at 18:20 reports eye-gaze extension
  advertised, D3D12 VRS Tier 2, 16-pixel tiles and additional shading rates.
  `XR_FB_foveation` and `XR_META_foveation_eye_tracked` are not advertised here.
  Capability logging does not enable foveation or prove valid gaze samples.
- [ ] If gaze is exposed, bind `XR_EXT_eye_gaze_interaction`, query eye-gaze system
  support, and verify tracked samples while worn. Handle calibration, permissions,
  stale/invalid gaze and head movement. Avoid storing raw gaze in routine logs.
- [ ] Prototype fixed foveation with a D3D12 Tier 2 shading-rate image before
  making its center follow gaze. Hook eligible world rendering passes, reset state
  for UI and incompatible passes, and verify both eye projections. This requires
  renderer integration; adding blur after rendering will not save scene shading.
  The first pilot is implemented and running: separate immutable per-eye masks,
  optical centers from asymmetric FOV, 1x1 center / 2x2 opaque-world periphery,
  and full quality for UI/transparency/post effects. Live reduced-shading draw
  counts confirm usage; user reports similar perceived performance. Verify
  artifacts/menus/dungeons and benchmark enabled/disabled at identical settings.
- [ ] Add gaze-driven full-quality center with a conservative margin for latency,
  coarser peripheral shading, configurable strength and safe tracking-loss fallback.
  Protect UI readability; test particles, transparency, post effects and rapid gaze.
- [ ] Benchmark foveation on/off with identical settings and document GPU and
  end-to-end frame-time changes. CPU work, geometry and full-size eye copies may
  limit gains. Do not infer performance from theoretical shaded pixel counts.
- [ ] Check Frame's foveated streaming settings separately: streaming quality/
  bandwidth optimization and game rendering optimization are distinct stages.
- [ ] Diagnose the ReShade sky/fog effect that has stayed at one remaining compile;
  restore sky/fog quality and measure its cost before setting the final preset.

Technical references checked for this investigation:

- [Valve custom-engine eye tracking and foveation extensions](https://partner.steamgames.com/doc/steamhardware/steamframe/engines/custom).
  These documents include standalone guidance; verify the streamed PC runtime.
- [Microsoft D3D12 VRS tiers and shading-rate images](https://learn.microsoft.com/en-us/windows/win32/direct3d12/vrs).
- [Proton WineOpenXR generator](https://github.com/ValveSoftware/Proton/blob/proton_11.0/wineopenxr/make_openxr).
  Source compatibility is not a substitute for querying the installed runtime.

## Established baseline

- [x] Native OpenXR D3D12 stereo transport works through Proton/SteamVR to Frame;
  user confirmed offline first-person gameplay and captures show Nekro in camp.
- [x] Normal Steam Play launches stock D2R; VR is an explicit separate launch.
- [x] Preserve Steam authentication/runtime context and the original executable.
  Legitimate unmodded online license refresh is separate from offline mod testing.
- [x] Back up replaced files and saves; retain a documented restore path.
- [x] Preserve upstream MIT attribution and keep the original D2R-3D checkout separate.
