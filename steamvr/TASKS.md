# D2R Steam Frame PCVR tasks

Updated 2026-10-09. Target: first-person D2R on this Linux PC through Proton,
SteamVR streaming to Steam Frame. Test with offline **Nekro** or the owner's new bow-test Amazon only.
The DevilutionX standalone/tabletop project remains parked separately.

Unchecked tasks remain open, including implemented changes awaiting headset
verification. Build checks and automated tests do not establish visual correctness.

User verified the changes so far in the 18:20 build, then reported inventory
closing on laser clicks and no visible belt. Individual regression scenarios
below remain explicit so they are not lost after that general check.

Current priority: **motion-controlled VR gameplay**, following the owner-verified
lighting, label and eye-tracked foveation work. See [GAMEPLAY.md](GAMEPLAY.md)
for the first installed prototype, exact controls, limitations and test order.
Broader performance/visual regression checks remain below.

## 1. Stability and calibration — next headset pass

- [x] Restore first-person stereo after the 20:39 saved mode switched to third
  person. At 21:04, changing only `vr_mode=4` fixed the camera pivot and stereo;
  owner confirmed both normal, ReShade effects still enabled.
- [x] Install the native-only first-person guard: repair old saved modes,
  ignore unsupported F1–F3/F12 camera switches, retain flat-mode selection.
  Installed after Save & Exit at 21:09; live saved-mode repair and F2 rejection
  checks passed. Broader gameplay/flat-mode visual regression remains open.

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

- [x] Fix ordinary creature name/health stereo depth. Centered projection removed
  large separation; drawing the health panel after current-eye tooltip updates
  fixed reversed disparity. Corrected build installed 23:03; owner **“fixed :)”**.
  Both fixes now default on; DLSS off is retained for clarity. See HANDOFF.md.
- [ ] Broaden label checks to close/distant creatures, head movement, bosses,
  rapid target changes, NPCs and ground items. Historical investigation follows.
  Earlier evidence: user reported doubled labels on normal monsters (name and health in one label).
  The new unit-label projection hook retains native world anchors and uses the
  exact current-eye scene matrices, avoiding inline flat-frustum rebuilding.
  Stereo anchor/ray convergence checks pass; they alone did not fix the visuals.
  The first matrix-only prototype still doubled in the user's test (native
  projected positions differed by less than a pixel). The next build combines
  the original foot X/Z and head height in world space before projecting once,
  replacing native projected-foot-X/projected-head-Y mixing. Verify this new
  anchor with head pitch/roll; the user tested it and **labels still double**.
  Stop treating projection math tests as visual proof. Next inspect single-eye
  label draws, cached UI layout, rectangle placement and eye/frame pairing.
  The 20:16 paired Fallen Shaman capture shows one box per image at the other
  eye's position. TooltipsPanel updates the separate MonsterHealthPanel during
  drawing, after it may have painted already. The 20:33 deferred-paint prototype
  was withdrawn during camera diagnosis and archived in the stereo-regression
  backup. The camera failure was subsequently isolated to saved third-person
  mode; this does not validate the prototype. Instrument actual paint/event
  ordering and renderer state in first person before retrying. Check target
  changes, NPCs, bosses and menus as well as convergence.

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
  At 21:35 another failure was isolated to desktop focus: a terminal was active,
  while movement continued through XR. Restoring game focus restored Left X
  (owner confirmed). Remove that gate from the native D2R UI action; keep OS
  mouse/key injection gated, request focus only for deliberate menu actions.
  The fix was installed at 21:40 after Save & Exit. Intentional focus loss now
  permits native inventory open/close, verified by the owner and logs. Laser
  input still needs desktop focus: the OS rejected programmatic focus recovery.
  The owner accepts this requirement and explicitly asked not to pursue it
  further. Documented in INSTALL/README; keep D2R focused for laser clicks.
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

- [x] Owner verified right-grip/button attacks, then rejected the interaction:
  no grip-to-arm step, preselected target or button simulation for physical melee.
- [x] Implement free-swing swept-capsule contacts with native exact-target basic
  damage on the authoritative thread, per-stroke deduplication and contact haptics.
  Category weapon volumes / inferred creature capsules are approximate.
- [x] Fix captured strong-slash lockout: bounded recovery without a still-hand
  requirement, rate-aware tracking guards, no derived upper-speed cutoff, and
  length-preserving arc sweeps and bounded stroke-onset history. Host replay
  improves 4 to 48 target/stroke contacts. Owner reported improved slashes;
  remaining fast-motion misses were addressed in revision 4 below.
- [x] Match melee input/render timestamps and use same-pose world
  geometry; synthetic delayed-render regression reproduces and fixes lost fast
  slashes. Revision 4 installed and owner verified **"works great"** on October 9.
- [x] Verify the free-swing sword baseline in Steam Frame. Final run recorded
  58 physical contacts / 58 native accepted hits, with no rejections.
- [ ] Extend headset checks across weapons/creatures, damage/procs and resets;
  refine dimensions/axes and add attack-speed scaling.
- [x] Implement the requested removal of random misses for the local player's
  melee, arrows and magic against creatures: exact hit-roll override plus random
  block/dodge bypass, with native eligibility/range/collision and damage/costs.
  `contact_hits=1`, dependent on motion controls; incoming attacks and summons
  retain native rules. Tests/build pass; three live melee overrides confirmed.
- [ ] Verify outgoing arrow/spell hit checks for deterministic hits. Preserve
  critical/damage/progression and resource systems; bow flight tuning is parked.
- [x] Implement the first right-handed motion-input prototype: deliberate melee
  swings, bow nock/draw/release, hand palette for eight native skill assignments
  and weapon swap, belt grab/raise/tip-to-drink, hand aim and gesture haptics.
  Portable gesture tests and cross-build pass. Bow drawing and free-swing sword
  combat are verified; palette, potion and spell interactions still need checks.
- [ ] Complete the remaining headset scenarios in GAMEPLAY.md. The owner has an
  offline bow-test character. Keep Nekro and both characters' inventory/progress.
  First bow test: equipped hunter's bow (`hbw`) and arrows (`aqv`) were detected,
  but owner could not nock/draw/fire. No successful gesture events were logged.
  Revised build installed 00:01:59 October 9: removed left-grip prerequisite,
  uses a larger visible string-side grab region, allows squeeze-before-approach,
  trigger hysteresis, explicit B cancel, automatic bow mode on equip, a visible
  arrow/V-string guide and grab/draw haptics. Added rate-limited readiness logs
  to distinguish tracking/input gates from failed gestures. Tests/build passed;
  Owner subsequently confirmed drawing and shooting work, but the native bow
  draw and rumble happened only after trigger release. Read-only capture caught
  that delayed native wind-up. New procedural pose test drives the existing
  string/limbs/arrow while trigger is held; guarded to the hero's attached bow,
  fixed string/limb lengths from runtime bind geometry, unknown-rig fallback.
  Added stronger checked haptics and draw-progress pulses. Geometry/timeline tests
  and cross-build pass. Owner verified live native pose and haptics work.
- [ ] **Parked by owner, October 9:** improve bow aim alignment and perceived arrow travel speed. First isolate
  idle bow-marker frame submission errors (`xrEndFrame -39`, present in prior
  guide build too). Distinguish attack wind-up, travel velocity, ground-only aim
  and model-vs-hand-line alignment before changing combat values. Owner clarified
  sideways shots and requested a visible arrow exactly along both hand anchors.
  00:34 build implements native 3D nock/arrow posing through the full attachment
  transform, 400 ms release-direction latch, and omits native idle nock marker.
  First anchor build failed visually: generic attachment follows inventory slot
  (right), not the actual left bow. Corrected to explicit L_weapon_attach;
  built/installed after game closed and owner verified hand following. Idle-marker frame
  errors stopped in the 00:34 build's live test. Latest corrected build has 734
  live draw samples: string-to-hand p90 3.26 mm, arrow-to-hand-line p90 0.452 deg;
  no frame-submit errors. Owner confirmed both-hand following. Remaining issue
  clarified: flying arrow model angled across its roughly straight flight path,
  not the bow twisting. Add native projectile orientation correction and 2x
  travel-speed test. Flight test feedback: somewhat aligned, not much faster.
  Hooks installed but no creation/velocity counters appeared in the latest
  session; do not claim verified 2x speed. Leave flight tuning parked for melee.
- [x] Implement initial generic main-hand melee recognition and aim for every character
  class: weapon-category sensitivity, translation/rotation strokes, thrusts,
  net-excursion jitter rejection, ready haptic, 250 ms swing-bearing latch, and
  native-reach guard for staff/javelin swings. Host tests cover all four gesture
  profiles, three stroke directions and 36/72/120 Hz input. No class/rig special
  cases. This grip-armed prototype was superseded by the free-swing contacts above.
- [ ] Verify generic melee in headset across characters and weapon categories:
  correct target, damage, reach, timing, held model and menu/swap recovery.
  Sword contact and native accepted-hit feedback are verified; independent
  off-hand strikes/shield collisions remain open.
- [ ] Replace prototype slot text/flat bottle/draw-line guides with actual skill
  metadata, belt counts and 3D item presentation. Add wrist health/mana feedback.
- [ ] Add pitch-sensitive spell placement, parries, crossbow loading and throwing.
  Refine held-bow presentation across rigs and melee collider dimensions.
  Damage/resources stay native, with the explicitly requested deterministic hit
  exception above. These remaining interactions need individual validation.

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
- [x] Superseded initial optional gesture-to-button approach: swing triggered the game's
  native attack, with thresholds/debounce and trigger fallback. Respect weapon
  reach, game attack speed, hit rules and cooldowns; no attacks from menu gestures.
- [ ] Add optional two-hand bow mechanics: nock/draw/release gesture and controller
  aim trigger the native shot. Preserve ammunition, native damage and attack timing.
- [ ] Define aimed spell casting, self-cast/buff and summon controls separately;
  preserve mana costs/cooldowns and an accessible button alternative.

## 4. Performance and eye-tracked foveation

- [ ] Parked by owner: SteamVR GPU failure after Frame standby/reconnect at
  22:45 on October 8 (NVIDIA Xid 32 in vrserver, followed by watchdog/compositor
  failures). Evidence in HANDOFF.md. Owner requested visuals take priority;
  no crash-specific code/driver changes applied.
- [ ] Validate DLSS stereo reconstruction before recommending it. Owner enabled
  Balanced at 21:48 and reported distorted visuals plus a temporary pause. Found
  and fixed a MinGW/MSVC NGX overload-vtable mismatch: resource/scalar reads and
  vector replacement were calling incorrect slots. All 17 adapter calls now
  match compiler-generated MSVC SDK calls; built and installed at 21:55.
  Live test confirmed valid input reads, then exposed a second blocker: the
  diagnostic statistics size limit skipped correction at 4K Balanced. Fixed
  that guard to apply only to statistics; installed and relaunched at 22:04.
  At 22:05:39 logs confirmed correction submission for both eyes at 2227x1253.
  Owner confirmed **“looks good”** with Balanced after the second fix. Compare
  performance with matched scenes before claiming a gain. Preserve
  installer default DLSS off. Correction compilation was separated from diagnostic
  reduction at 22:30: isolated compilation dropped from 22.9 s to 33 ms with
  identical correction instructions and no group-shared allocation. Installed;
  live 22:31:57 correction compilation/pipeline creation took 67 ms. Measure
  steady-state results separately from ongoing walking hitches. Following the
  centered-projection fix, the owner turned DLSS off and reported **much clearer**.
  Preserve DLSS off locally as well as in installer defaults; sharpness/temporal
  reconstruction still needs work before recommending DLSS.
- [ ] Fix distant square lighting/highlight patches differing between eyes on
  waypoints and clothing. Not visible close up. Still visible with DLSS off,
  added sky/fog off, and bloom upsampling suppressed; all diagnostics restored.
  Native light-bin shader ignores off-axis projection terms; a live centered
  projection fix is installed at 22:42 (`render.symmetric_projection`, now default 1).
  Owner reports **patches seem gone, image softer** after the waypoint test.
  Subsequently disabling DLSS made the image **much clearer**.
  Keep local setting 1; validate clothing/other scenes and improve sharpness
  within the performance budget before closing. See HANDOFF.md. Preserve
  view-dependent highlights and right-eye particle timing while investigating.
- [ ] Investigate residual bursts after repeated background-load interference
  ended around 21:46. Owner said movement felt much better with a slight burst
  remaining. D2R CPU-affinity trials 40–43 were confounded by scene/load changes;
  original affinities restored. Sequence 44 actually captured DLSS startup and
  must not be presented as an affinity result or matched performance comparison.
- [x] Identify the current bursty walking regression's CPU contention. A parallel
  Iron Signal test started at 21:23:45 and used about 18 logical CPUs. The owner
  authorized a 45-second pause with automatic resume and asked its agent to stop
  performance work. After it stopped, the owner confirmed **“Smoother, bursts
  gone.”** Walking sequence 31 averaged 29.4 ms, p95 37.1, maximum 41.8 over 360
  pairs; this is not a matched-scene percentage benchmark. Original sync remains
  enabled (`defer_gpu_wait=0`). No process was left paused. Inspect competing
  workloads before future comparisons; do not pause other projects unasked.
- [x] Add bounded stage/callback/GPU profiling and capture stationary and moving
  gameplay. Sequences 1/3 show mean cycles of 27.0/48.5 ms, with 16.3/25.0 ms of
  drawing-thread CPU per pair. Effects cost about 1.4 ms and copies 0.05 ms per
  pair; GPU scene spans include CPU starvation. These are different workloads,
  not a before/after benchmark. Worker threads remain outside callback probes.
- [ ] Validate deferred GPU completion waits during gameplay, menus and exit.
  Installed at 21:27; copy allocator reuse and shutdown retain completion checks.
  Repeated 1/0/1/0 comparison (sequences 20–23) has mean cycles 35.8/39.7 ms and
  XR end 3.85/5.72 ms. Head directions differed, so do not claim a controlled
  percentage gain. Some removed fence wait moves into runtime submission.
  Owner reported bursty walking with this enabled; restoring 0 did not change
  movement. Leave owner configuration and source default at 0. An unrelated
  Iron Signal test was subsequently found using about 18 logical CPUs; exclude
  competing workloads before drawing performance conclusions.
  Use `profile_gpu=0` for comparisons, as GPU readback deliberately forces a wait.
- [ ] Attribute engine scene preparation and worker-thread work inside the two
  eye passes. Sampled main-thread mod callbacks explain only a small fraction;
  that does not exclude unmeasured mod work. Keep the long pre-render hitch
  investigation separate from steady-state drawing cost.

Next investigation order after the camera repair (21:11 measurements):

1. Attribute both eye passes' CPU cost before changing visual settings. Recent
   windows use 21–22 ms of render-thread CPU per pair, with 28–30 ms total eye
   pass wall time. The RTX 5090 was at 30% utilization in one spot sample (not
   a benchmark). Time mod callbacks/scans and engine scene preparation separately.
2. Measure GPU work and OpenXR synchronization, including the now-active
   ReShade effects, eye acquire/copy/fence/release and compositor submission.
   Recent hitch records include 10–15 ms around XR end, but these are not
   representative average costs. Do not remove resource completion guarantees.
3. Attribute long pre-render stalls separately: at 21:11:30 a 633 ms cycle
   spent 581 ms before stereo drawing, near native UI/input events, with zero
   tracked pipeline creations. Loading, UI and mod work need separate timing;
   neither shader compilation nor asset loading is established by this sample.
4. Once measured, investigate sharing eye-independent scene preparation or
   shadow work across the two eye draws. This is substantial engine work;
   never skip animation/UI updates blindly or reuse eye-dependent culling.
5. Compare shadows/VFX/atmospherics, eye resolution and foveation at a fixed
   scene only if GPU timing identifies them as limiting. Preserve a visual
   baseline and measure both frame-time tails and average pair time.


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
- [x] Reduce foveation CPU callback overhead. Cache command-list and repeated
  pipeline state, defer VRS changes until a draw, batch draw counters and handle
  indirect/mesh draws conservatively. User's 19:49 test: **“Smoother”**. CPU time
  now measures correctly under Proton; observed render passes are 94–97% CPU
  busy while GPU utilization has a median around 43%. No precise gain claimed.
- [x] Resolve ReShade's persistent shader compilation and verify its CPU cost.
  A worker used one core throughout the prior session. Trial the current SDK
  shader compiler privately under the mod, retaining stock launch and prefix
  libraries. The older Winetricks compiler does not support ReShade's SM 5.1.
  Current SDK compiler completed the full effect in 10.56 seconds, sky now renders,
  and the sustained compiler worker is gone in the subsequent thread sample.
  Previously missing effects now run, so this is not an equal-quality FPS test.
- [ ] Attribute the remaining half-second gameplay stalls. One 504.7 ms cycle
  had a 474.1 ms pre-render gap, main-thread CPU activity and ~27 MB cached reads,
  without physical reads. Asset processing is a candidate, not established cause.
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
  October 8 23:21 pilot: system support and tracked timestamped samples confirmed
  in the actual SteamVR/Proton session after the owner approved enabling
  `driver_vrlink/shareEyeTrackingData`. The setting defaults off; never change it
  silently on a friend's machine. Binding, validity/freshness/recovery fallback
  are implemented; calibration/loss/reconnect coverage still needs testing.
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
  Prototype installed: 9x9 immutable mask bank, gaze radius .35 + .177 grid margin,
  shared between eyes and cached per viewport/radius, one-time 16 ms setup in the
  owner's 3840x2160 scene. Valid gaze and reduced-shading draws are confirmed;
  owner confirmed clear/stable eye/head movement and labels. `[render]
  eye_tracked_foveation=1` opts in before launch;
  defaults off for distribution. Invalid gaze uses full-quality shading.
- [ ] Benchmark foveation on/off with identical settings and document GPU and
  end-to-end frame-time changes. CPU work, geometry and full-size eye copies may
  limit gains. Do not infer performance from theoretical shaded pixel counts.
  First stationary gaze/full/gaze comparison (360 pairs each) averaged
  22.566 / 23.240 / 22.619 ms per pair, ~2.8% lower with gaze VRS. GPU scene spans
  were similar and include CPU queue starvation; this is a small preliminary
  result. Compare fixed versus gaze and test dungeons/heavier scenes next.
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

## 6. Headset recordings

- [x] Install Framecorder 0.1.1 on Frame and verify a short recording. Dashboard
  service works; 4-second 1080p/90 fps HEVC headset-view recording was inspected.
  Game audio is enabled; microphone and instant replay are off for performance
  testing. Record/Stop are in the Framecorder SteamVR dashboard tab.
- [ ] Optional direct-panel capture: owner enters Frame administrator password
  locally for `sudo -v && ~/.local/bin/framecorder-setup --unlock`. Current
  headset-view recording works without this. Keep passwords out of chat/logs.

## Desktop presentation

- [ ] Investigate an optional disabled desktop mirror and measure its actual
  overhead. Current monitor flicker comes from alternating left/right eye
  backbuffers; there is no separate monitor scene render. ReShade addon Present
  callbacks cannot cancel the OS presentation. Preserve game frame advancement,
  headset capture, menus and controller focus; minimizing is not a substitute.
  A stable single-eye mirror is an alternative but adds a GPU copy in the
  current design. No display-path changes have been installed.

### Basic-arrow flight test, 2026-10-09

- [x] Locate static flying-arrow model and native half-second release-point blend.
- [x] Build local-player-only basic-arrow 2x velocity hooks on client and server;
  no game table/save/asset edits. Native lifetime means possible longer range.
- [x] Build selective native release-point component removal before handoff.
- [ ] Verify actual client/server velocity events and direct handoff in game.
- [ ] Headset check: arrow axis along flight, faster flight, normal enemy hits.
- [ ] Extend to elemental/skill arrows after basic-arrow test; current scope is id 0.
