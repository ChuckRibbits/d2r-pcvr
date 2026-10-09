# Agent handoff — D2R PCVR

Updated 2026-10-08. Read [INSTALL.md](INSTALL.md) for deployment and
[steamvr/TASKS.md](steamvr/TASKS.md) for the complete user-requested backlog.

## Objective and boundaries

First-person Diablo II: Resurrected PCVR. Original user reference was
`emmericp/D2R-3D`; the adapted implementation is MIT upstream
`natpoh/diablo2-vr-port`, commit `10cfb0a2a9080184101794d3ac8c238c35717967`.
This repository retains upstream history/notices and adds a native OpenXR
transport, without BodyWalk/FlatVR as runtime dependencies.

Owner tests on Linux with Proton Experimental, SteamVR and Steam Frame. His
offline character is **Nekro**. Friend has Quest 3, likely Linux with **WiVRn**;
Windows with Virtual Desktop is also possible. Both friend paths are untested.
The instructions deliberately identify that rather than promise compatibility.
Earlier DevilutionX standalone/tabletop and first-person work is parked and is
not part of this repository or deployment.

**Offline single-player only.** Never use modded online characters, bypass license
verification, change the stock executable, remove equipment-bearing corpses or
overwrite progress. Stock Steam Play must stay available for legitimate online
license refresh; the explicit `--frame-vr` launch chooses D2RLoader. Save and exit
before replacing DLLs or restoring files. The installer checks for running D2R.

## Latest user evidence and priorities

Latest performance evidence: after another competing Iron Signal workload was
stopped around 21:46, the owner reported much better movement with slight residual
bursts. The earlier 21:39 **“Smoother, bursts gone”** report was followed by more
contention from a new process. Check current host load, not just the original PID.
At 21:48 the owner enabled **Balanced DLSS** and reported distorted visuals and
a temporary pause. NGX parameter ABI and correction-size-limit bugs were repaired;
the **22:04:32** build logs correction submission for both eyes. The owner then
confirmed **“looks good”** with Balanced DLSS. They also reported subtle left/right
differences in emissive/other effects that may predate DLSS (details below).
Keep `[render] defer_gpu_wait=0`; the experimental synchronization change did
not explain the bursty motion. See the capture results and limitations below.
After the 22:42 installation, the centered-projection lighting trial received the owner report:
**“patches seem to be gone. Though the image is a bit softer.”** Keep the installed
`[render] symmetric_projection=1`. This supports the native light-bin off-axis
projection diagnosis; broader scene/clothing checks remain. The wider rendered
FOV at unchanged texture resolution explains an expected sharpness tradeoff.
After the 22:48 relaunch the owner turned DLSS off and reported **“much clearer.”**
Current visual baseline is therefore **DLSS off**, centered projection 1, fixed
foveation 1 and conservative synchronization 0. Do not restore Balanced on a
future relaunch; the earlier Balanced preference is superseded. Settings.json
confirms NVIDIA DLSS=0; the scene log switched from 0.580 to 1.000 at 22:49:51.
The comparison shows DLSS contributed to softness; do not attribute all blur
to the centered projection or treat this as proof the DLSS correction is flawless.
The user also reported **“Smoother”** for the 19:49 build on 2026-10-08, which
caches foveation command-list/pipeline state, defers redundant
VRS binding, batches diagnostic draw counts and fixes CPU accounting on Proton.
The user explicitly prioritized **performance first, creature labels second**.
The source checkpoint includes these fixes. The published v0.1.0 preview ZIP
still predates them; build current source or publish a new preview before telling
a friend the downloadable binary contains the lighting/label corrections.

1. **Residual performance hitches and CPU rendering overhead.** The logger's
   disk stalls and repeated foveation callback work have been reduced. Do not
   claim all remaining stalls are shader compilation.
2. **Broader creature-label regression checks.** After centered projection and
   current-eye health-panel draw ordering, the owner confirmed **“fixed :)”**.
   Both fixes are now enabled by default. Bosses, rapid target changes and a
   wider range of distances still deserve checks; no need to repeat failed
   anchor-only experiments. Details and limitations below.
3. **Validate Quest 3 + WiVRn**, or Windows + Virtual Desktop, using the friend's
   agent and real headset. Check runtime extensions before changing the transport.
4. Finish menu regressions, targeting/loot feedback, on-demand belt/health/mana,
   weapon/spell selection and optional melee/bow mechanics from TASKS.md.

## Implementation map

| Area | Files |
|---|---|
| Native OpenXR, eye copies, controllers, recenter, menu laser | `steamvr/transport.cpp`, `math.h`, `input.h` |
| Fixed foveation / D3D12 Tier 2 VRS | `steamvr/foveation.cpp`, `foveation_math.h`, `vrs_compat.h` |
| Buffered runtime logger | `steamvr/logging.cpp`, `logging.h`, `logging_test.cpp` |
| First-person camera, per-eye label hooks | `cleanroom/camera/d2rcam.cpp`, `mat4.h`, `game_layout.h` |
| Native UI panel toggles / game state | `vr/gamestate.cpp`, `vr/vrcam.cpp` |
| Body selection / checked scanning | `vr/skeletons.cpp`, `vr/probe.cpp`, `steamvr/hero_selection.h`, `memory.h` |
| Room/draw-distance and fog scans | `cleanroom/drawdist/drawdist.cpp` |
| MinGW/MSVC ReShade aggregate-return ABI adaptation | `external/reshade/include/reshade_api*.hpp` |
| Install/restore, Linux and Windows Steam wrappers | `steamvr/install.py`, `launch.sh`, `session.py`, `launch_windows.py` |
| Portable binary preview packaging | `steamvr/package.py`, `cmake/openxr-loader/` |

## Creature-label investigation

**Verified result:** the owner confirmed **“fixed :)”** after the corrected
draw-order build at 23:03. Centered projection and current-eye health-panel draw
ordering are enabled by default for native OpenXR and in the installer. DLSS
remains off. A subsequent 23:05:10 pair confirms both eye paint callbacks execute
after UI updates, but caught no creature, so it is not a quantitative post-fix
label-disparity measurement. Capture counters are reset to 0.

Earlier diagnosis: centered projection removed the large separation in the owner's
view, but they clarified **label depth seems off**. Capture **22:54:42**, pair 1,
shows a Zombie label centered at **X=2030 left / 2049 right** in 3840x2160 images:
the box shifts right while the creature shifts left. Corresponding native unit
anchor X values were 2037.20 left / 2021.59 right. Thus large apparent doubling
has improved while the remaining disparity still has the wrong sign.

A live comparison with only `stereo_labels=0` produced **“About the same.”**
Restored 1, reset capture_labels=0. Second local pair is **22:56:27**. This
does not implicate the unified-head correction as the source of residual depth.
Keep centered projection 1 and DLSS off through this investigation.

A fresh draw-order fix is built, now default `[hud] monster_draw_order=1`.
It reinstates the narrow deferred-health approach after read-only verification:
native PanelManager::Draw RVA **0x89F2E0** delegates to Widget::Draw **0x855B00**
(visibility and child traversal); it does not end the caller's UI render pass.
D2RLoader's vtable thunk is retained. Native panel-draw signature is checked in
addition to widget/tooltip signatures and both expected vtable entries. The
trial is scoped to the render thread's current stereo gameplay eye, queues no
widgets across traversals, and can be disabled live after installation. It is
independent of the anchor-correction setting. A capture logs health-panel paint
after native tooltip updates for each eye. No label textures/health values are
replaced. Local disassembly evidence is ignored `build-frame/{widget_draw,
panel-manager-native,panel-manager-loader,tooltip_draw}.asm`.

Host camera and deferred-order tests plus the cross build pass. New DLL SHA256:
`477d2b2d55f7440b297e36909c22e37ea486f9ede38c699740a6f6066d5c513b`.
Installed after Save & Exit at **23:00:55**, then relaunched. Backup:
`build-frame/backups/label-order-centered-20261008-230055`. Installed setting
monster_draw_order=1; centered projection 1, stereo_labels 1 and DLSS off retained.
Startup at 23:01:03 kept native paint order because the added PanelDraw signature
omitted its leading 0x40 prefix. No draw-order correction ran in that launch.
Corrected the byte pattern and verified all three complete signatures against
the live process (including the prefix), then rebuilt successfully. Corrected
DLL SHA256: `2eb6287946b1eeee02868cfb63c0baff936cc3625b10fd168f7d54100d1d7781`.
Installed after Save & Exit at **23:03:15**, backup
`build-frame/backups/label-order-signature-20261008-230315`.
Startup **23:03:23.269** confirms the hook installed; stereo gameplay resumed at
23:03:43. The owner then confirmed the depth fixed. Camera/queue logic is
unchanged; do not describe the first launch as a label-depth comparison.
The subsequent source build changes only missing-setting defaults to match the
already tested installed values. The running DLL need not be replaced for that.

Game build **3.3.93787**. Signature-guarded optional hooks:

- UnitToScreen RVA **0x76A7D0**, signature
  `4c 8b dc 55 56 57 41 57 49 8d 6b a8 48 81 ec 38 01 00 00`;
  `bool(context, unit, float output[2], uint8_t above)`.
- Camera::WorldToScreen RVA **0xED5EB0**, signature
  `48 8b c4 48 89 58 10 57 48 81 ec 40 01 00 00`;
  `bool(camera, float point[3], float output[2])`.

The native unit path resolves the HD model transform/bounds, projects head and
feet, then combines **projected foot X with projected head Y**. Inline camera
projection rebuilding can bypass the normal projection hook. The first fix used
the exact eye scene matrices in WorldToScreen, scoped only to unit labels:
native/exact projections differed by under one pixel and labels still doubled.
The second fix captures original head/base world points, combines base X/Z with
head Y into one world anchor, projects once per eye, and replaces output XY.
It corrected horizontal placement by roughly 20–40 px but **still doubles**.

`[hud] stereo_labels=1` enables these current experimental hooks. Behind/near/
invalid points are culled. Menu and shadow fitting projections are unaffected.
Full viewport dimensions, asymmetric FOV, row-vector / row-major / right-handed
-Z / reverse-Z math are covered by tests. Passing them is not visual proof.

F8 requests the next complete stereo gameplay pair through ReShade screenshot
capture; both images share a `Frame-pair-N` suffix with left/right labels. The
log records bounded unit-anchor data, eye FOV and pose for that pair. A remote
agent can instead increment positive `[debug] capture_pair` in the installed
INI while the game runs. A request is consumed only during active stereo
gameplay. A positive `[debug] capture_labels` arms a one-shot capture after a
visible native above-unit label is observed; changing that counter rearms it.
Readback causes a diagnostic pause: exclude that pair from benchmarks.
Screenshots contain game imagery and must remain local/ignored.

The automatic capture at 20:13:51 caught **Kashya**, one clear label per eye.
At **20:16:28/29**, pair 2 caught **Fallen Shaman**: the creature shifts left in
the right eye, but its health box shifts right. UnitToScreen returns native X
2429.48 (left) / 1779.95 (right) on a 3840-wide viewport. The rendered box instead
tracks the preceding eye's position. Each image contains one box, not two.

Code evidence: `TooltipsPanel::Draw` **0x880550** calls the target-name routine
**0xC0420** at 0x880AA5/0x880AE3/0x880B0C. That calls helper **0xCE850** and
monster information routine **0xCEA50**, which sends a UI event carrying the
current anchor. `MonsterHealthPanel::OnEvent` **0x1F8E60** updates its
`TargetAttached` and `ScreenAnchored` children via **0x1F84E0**. The attached
child receives the packed anchor at event payload +0x30 through **0x857050**.
The independent health panel can already have painted before TooltipsPanel.

The **20:33 draw-order prototype was withdrawn**, not validated. It patched
PanelManager paint (0x1D79CE8, original D2RLoader thunk 0x3E2B7F6) and
MonsterHealthPanel paint (0x1CDF3D0, generic 0x855B00), queuing the health widget
until the root traversal returned. A CPU queue test and startup signature checks
passed, but do not establish renderer-state or ABI correctness. Source and DLL
are archived locally in `build-frame/backups/stereo-regression-20261008-205705`;
source was removed from the working tree. Do not reinstall that DLL blindly.
The preceding DLL/effect were restored from
`build-frame/backups/monster-ui-order-20261008-203340` at 21:00.

The owner reported broken stereo, incorrect head pivot and third-person view.
Restoring that DLL did **not** fix it. The log shows camera mode changed to
third person at **20:39:05.943**, persisted as `[mode] vr_mode=2`, and reloaded
at 20:53 and 21:00. At **21:04:35**, changing only `vr_mode=4` restored first
person. The owner confirmed **“Both are normal now”** for camera pivot and
stereo convergence, with ReShade effects still enabled. The input that triggered
the mode change is unknown; F2 and shared legacy view actions both reach SetView.
Only the first-person camera uses native OpenXR eye poses; other modes had
submitted incompatible camera images with the headset poses/FOV.

A native-only guard now repairs saved VR modes to first person and rejects
legacy mode-switch shortcuts, preserving flat mode and the FlatVR build.
The guard DLL was installed after Save & Exit at **21:09:14** and relaunched.
Backup: `build-frame/backups/camera-guard-20261008-210914`. Live regression
checks passed: writing `vr_mode=2` was repaired automatically at 21:09:50, and
F2 was ignored at 21:09:51 with the saved mode still 4. Startup confirmed native
OpenXR and compiled ReShade effects. DLL SHA256:
`78dcb5a53b688310a26e442812997a071b63b0b8093f51efeb36971e7b5ea140`.
The installer explicitly writes mode order 2.
No ReShade-effects A/B was necessary after this controlled mode repair.
Scroll Lock remains configured as the global ReShade effects toggle but was
not pressed. Debug capture counters are reset to zero.

At that earlier stage creature labels remained unresolved. The mode error means the withdrawn build
was not evaluated in the intended camera mode; it neither proves nor disproves
the draw-order approach. Instrument health widget paint/event ordering in first
person before reintroducing it, including validation of UI renderer state.

UnitToScreen callers found: 0xCE877, 0x198E90, 0x198FCE, 0x1FABDF (items),
0x6D7B15, 0x1519482, 0x1519875. Helper 0xCE850 applies scaled UI offsets.
The ReShade UI trace (`vr/uitrace.cpp`, Ctrl+F10) can dump one GPU frame and raw
targets. It causes a capture pause; do not mix it into a performance comparison.
UI target is R16G16B16A16_FLOAT with **inverse alpha**, cleared black/alpha 1.
Do not infer duplication merely from different left/right pixel positions:
the headset's asymmetric optical centers produce large legitimate differences.

## Performance findings

A bounded profiler was installed at **21:19** (backup
`build-frame/backups/frame-profile-20261008-211901`). It leaves all graphics
settings unchanged. Set `[debug] profile_capture`
to a new positive integer for 360 active stereo pairs; reset to zero afterwards.
`steamvr/profile_report.py` parses the log and exports CSV. See README for
measurement limits: main render thread only, inclusive callback scopes (hot
callbacks sampled 1/64), GPU scene queue spans include CPU starvation. No
worker-thread attribution is provided by these callback measurements.

The first captures contain 360 valid GPU samples each. Sequence 1 was standing
still; sequence 3 was genuine movement (65 distinct logged hero positions).
Sequence 2, misleadingly saved as `profile-walking.csv`, was stationary: do not
use it as a walking benchmark. Local artifacts are under ignored `build-frame/`.

| Measurement (ms per stereo pair unless stated) | Standing, sequence 1 | Moving, sequence 3 |
|---|---:|---:|
| Mean full cycle | 27.027 | 48.474 |
| 95th percentile cycle | 34.823 | 80.985 |
| Both eye passes, elapsed | 16.938 | 34.913 |
| Main drawing thread CPU in both passes | 16.306 | 25.000 |
| GPU completion wait | 3.721 | 3.534 |
| ReShade effects, both eyes | 1.416 | 1.454 |
| Eye copies, both eyes | 0.046 | 0.048 |

These are different scene/motion workloads, not an optimization A/B. Eye drawing
dominates, while copies and effects are comparatively small. Sampled main-thread
VRS callbacks total roughly 0.6–0.9 ms per pair. The rest is not automatically
engine-only cost: uninstrumented mod work and worker threads remain possible.
The GPU scene timestamp span includes queue starvation and is not GPU busy time.
No half-second stall occurred in these bounded captures; the separate 581 ms
pre-render stall still needs attribution. Read-only process sampling is saved as
`frame-profile-process.csv`; kernel perf stack sampling was unavailable under the
current `perf_event_paranoid=4` setting. No global permissions were changed.

A synchronization comparison DLL was installed after Save & Exit at **21:27:59**:
SHA256 `1727c9f4fe30f8f7571605797e85dc38a5175e9d35c3e8d0205bf889bd092f19`.
Backup: `build-frame/backups/deferred-wait-20261008-212759`.
`[render] defer_gpu_wait=1` submits image copies/state transitions on the D3D12
queue bound to OpenXR and checks each copy allocator's previous fence before
reuse, instead of unconditionally blocking before image release. Shutdown still
waits for resource completion. The runtime orders access on the bound queue;
Proton's bridge drains pending submissions before its release barriers. Source
default remains **0**. Active GPU timestamp profiling deliberately retains the
old completion wait; use `[debug] profile_gpu=0` for this synchronization A/B.

Sequences 10–13 first compared 0/1/0/1. The owner reported moving at the start,
confirmed in sequence 10, so the test was repeated as **1/0/1/0**, sequences
20–23, from 21:31:25 to about 21:32:30. Each sequence contains 360 pairs, with
the hero remaining at `(8740.6, 11277.0)`. Viewing direction varied, particularly
in the conservative runs; do not present the aggregate difference as a controlled
percentage speedup.

| Repeat captures, 720 pairs per mode | Conservative (21, 23) | Deferred (20, 22) |
|---|---:|---:|
| Mean cycle, ms | 39.671 | 35.831 |
| Median cycle, ms | 36.451 | 32.123 |
| 95th percentile cycle, ms | 63.505 | 59.101 |
| Mean XR end, ms | 5.721 | 3.846 |
| Mean GPU completion wait, ms | 3.418 | 0.000 |
| Mean runtime submission, ms | 1.705 | 3.082 |

Some removed wait reappears in runtime submission; the full 3.4 ms is not a net
saving. The combined XR end stage is about 1.9 ms shorter in this sample. This
supports a modest improvement, not a solution to rendering cost or long hitches.
No new XR/D3D12 failure or copy timeout was logged during the comparisons.
At 21:32 the owner tested `defer_gpu_wait=1` and reported bursty slow/fast movement
and Left X not opening inventory. The setting was restored to **0** live; the
owner said movement was unchanged. Do not treat deferred waits as a validated
improvement or enable them by default. Subsequent inspection found an unrelated
Iron Signal test consuming roughly **18 logical CPUs on this 32-thread host**,
another substantial confound for the comparisons. Its executable was
`/tmp/iron-x11-target/release/iron-signal-desktop`, PID 3286587. The owner explicitly
authorized a 45-second pause/resume comparison; results follow below.
No graphics settings, effects, saves or game executables were changed for the test.

The Iron Signal workload began at **21:23:45**, just before the synchronization
comparisons. At 21:37:33 it was paused with an independent 45-second resume
watchdog. The owner also asked the other agent to stop its performance work.
At 21:38:18 the pause ended; the original process subsequently exited. No process
was left stopped. New/lighter Iron Signal and compiler jobs were still observed,
so sequence 31 is **not** a repeat with the same heavy workload resumed.

| Capture (360 pairs each, conservative sync) | Paused, sequence 30 | Heavy process gone, sequence 31 |
|---|---:|---:|
| Logged hero positions | 1 (stationary) | 42 (walking) |
| Mean cycle, ms | 33.519 | 29.400 |
| 95th percentile, ms | 37.923 | 37.120 |
| Maximum, ms | 55.213 | 41.831 |

The owner then confirmed **“Smoother, bursts gone.”** CPU contention was a major
contributor to this regression. Scene/motion and competing workloads changed,
so do not derive a precise percentage gain. Local CSVs:
`build-frame/profile-contention-30.csv` and `profile-contention-31.csv`.
All capture counters are reset to zero. Check live competing CPU/GPU workloads
before any future VR benchmark; request approval before pausing another project.

The inventory regression was desktop focus: `_NET_ACTIVE_WINDOW` was a terminal,
and native Left X was unnecessarily gated on Win32 foreground focus, despite the
XR session remaining focused. Restoring the game window using `xdotool
windowactivate --sync` restored inventory; owner confirmed it and logs show
successful open/close cycles. `wmctrl -ia` alone did not actually focus this window.
The fix was installed at **21:40:18**, after the owner saved and exited, then
D2R was relaunched. Backup: `build-frame/backups/menu-focus-20261008-214018`.
Current DLL SHA256:
`357a5a0aa18f5c37dba624423ef53c9cc53ddee38992ae15c8bfbb178961a64e`.
It removes desktop-focus gating only from the native D2R menu action.
Explicit menu actions/clicks also try to focus the game;
OS mouse/keyboard injection still requires actual foreground focus, and passive
laser hover never steals focus. Build and existing input/math checks pass;
the intentional terminal-focus test confirmed native inventory opens/closes.
Windows/Proton rejected `SetForegroundWindow` in this test, so laser input was
correctly withheld while the terminal retained focus. The owner confirmed the
behavior and explicitly said **not to pursue a further fix**: requiring the game
window to stay focused is acceptable. INSTALL/README now state that requirement.
The game window was restored to the foreground after the test. Respect this
scope decision; do not add a focus broker or another workaround unasked.
Later at 21:40, different Iron Signal runtime/test processes again consumed
substantial CPU. Do not assume the host is idle because the original PID exited;
the owner has asked the other agent to stop performance work. No additional
processes were paused.

At 21:46 the heavy workload was gone and the owner reported much better movement,
with a slight residual burst. Temporary D2R-only CPU affinity trials (sequences
40–43, one 5950X L3 group versus original affinity) were confounded by changing
movement and the other workload stopping. Do not claim an affinity gain. All
original thread affinities were restored; no launch affinity change was made.
Sequence 44 started later than intended, after DLSS was enabled, and is saved as
`build-frame/profile-dlss-broken-44.csv`, not an affinity comparison. It includes
a 22.7-second first-eye CPU stall ending at the first DLSS correction shader
build; steady-state median is 30.75 ms, p95 34.53, but the mean includes that
startup pause. Do not compare this different scene directly with DLSS-off runs.

### DLSS compatibility trial, 21:55

The owner enabled Balanced through D2R's options (`NVIDIA DLSS: 3`); do not
silently overwrite it while the game runs. Per-eye NGX features were created,
but the motion-vector correction logged unreadable Depth/MotionVectors inputs.
`vr/dlss_mv.cpp` redeclared the SDK's overloaded virtual interface. MSVC reverses
each Set/Get overload group; our MinGW build does not. The old D3D12 resource
getter dispatched to byte offset 112 instead of 72, and its setter to 48 instead
of 8. `vr/ngx_parameter.h` now supplies unique virtual names in MSVC slot order
with nonvirtual typed wrappers. `steamvr/ngx_abi_test.py` compiles all 17 calls
against an independent SDK-order declaration with both Windows compiler targets;
the fixed code matches MSVC, and the old MinGW layout fails all 16 overloaded
calls. No external C parameter wrappers are exported by this `_nvngx.dll`.

Built and installed SHA256
`0a9df2d19fdff8a57f065567df5ec142a63752f6bc936e85dbbd42d9f0a1dd3d`.
Backup: `build-frame/backups/dlss-abi-20261008-215509`. Relaunched offline VR;
Balanced retained, `defer_gpu_wait=0`, profiling idle (`profile_capture=0`).
New logs report NGX input failure result codes or a correction submission for
each eye. This proves neither correct vector math nor acceptable visuals until
checked in the headset. Near-parallax correction remains upstream-default off.
The original shader still compiles synchronously on first DLSS use; the previous
session used the private Microsoft compiler and paused about 23 seconds here.
Installer default remains DLSS off until the trial is validated.

The owner restarted SteamVR during the first launch; relaunched at 21:57 with
Frame reconnected. At 21:58:18 the fixed ABI read valid resources: depth
3840x2160 R16_UNORM, vectors 3840x2160 R16G16_FLOAT, scene 2227x1253, MV scale
-2227/-1253. The first shader build again stalled about 22.5 seconds. Correction
still did not submit because the diagnostic statistics limit (900,000 bytes)
was incorrectly applied to mode 2, which never writes those statistics. The
next build moves that guard to the diagnostic path only. After the owner saved
and exited, installed at **22:04:32**, SHA256
`d8c298c663e10f261272a407af4bb4498bd535807529a5eb70d0b4048f1dcacd`.
Backup: `build-frame/backups/dlss-size-20261008-220432`. Relaunched with Balanced
retained. At **22:05:39** the log confirms valid inputs and correction submission
for **both eye 0 and eye 1**, scene 2227x1253. The initial shader build stalled
22.3 seconds again. The owner subsequently confirmed **“looks good.”** This is a separate
bug from the ABI.
A stereo capture at 21:59:14 is in the game directory (`Frame-pair-1-left/right`);
it is not a performance sample. `capture_pair` was reset to 0 afterwards.

Post-compilation sequence 50 (`build-frame/profile-dlss-corrected-50.csv`) has
360 pairs: mean cycle **25.946 ms**, median 23.282, p95 33.643, max 35.431.
Both eye passes total 16.832 ms wall / 15.973 ms render-thread CPU; XR wait
3.102 ms and GPU fence 3.477 ms per pair. No GPU timestamp capture was enabled.
This is a short current-scene observation, not a matched DLSS on/off benchmark.
`profile_capture`, `capture_pair` and `capture_labels` are all 0 again.

### DLSS correction specialization, 22:30

Separated the production correction shader from the diagnostic statistics shader
at compile time in `vr/dlss_mv.cpp`. Pipelines are created on demand and retained
independently, so normal mode 2 never compiles the diagnostic reduction. The old
uniform branch reserved 27 KiB of group-shared memory even for correction-only
work. The new correction declares no group-shared storage or group barriers.

An isolated Microsoft SDK compiler probe under a separate Wine prefix measured
**33 ms / 4,632 bytes** for correction versus **22,880 ms / 52,028 bytes** for the
old combined shader. Diagnostics still compile in about 22.7 seconds, only when
explicitly selected. Compiling the old shader with its correction branch fixed
true produces **exactly the same shader instructions** as the new production
shader. Artifacts: ignored `build-frame/dlss-shader-probe.log` and
`dlss-{old-0,new-0,new-1,reference-0}.asm`. The cross build passed. This establishes
shader compilation improvement, not a measured in-game FPS gain.

Installed after Save & Exit at **22:30:59**, backup
`build-frame/backups/dlss-specialized-20261008-223059`. DLL SHA256:
`4471de6d4b0de9c9d153bbeccf7dbbf941fdcacf427fb224dc2ef3f92bc457dd`.
The owner had left DLSS off after the diagnostic comparison; restored only that
setting to their original Balanced (3), then relaunched using `--frame-vr`.
Live validation at **22:31:57.798** logged the 4,632-byte correction pipeline
ready in **67 ms**, followed by correction submission for both eyes at
2227x1253. The initial pair was 95.8 ms rather than the earlier roughly 23-second
stall. This confirms the startup fix; steady-state FPS has not been compared.

### Distant stereo lighting patches (initial centered-projection check passed)

The owner clarified: glow/emitted light around waypoints, portals and lamps,
also highlights on clothing, show different square patches in each eye and
flicker with head turns. Flames themselves look fine. The fault disappears
close up and is visible at a distance. Controlled observations this session:

- Added sky/fog/ceiling disabled together: **still flickers**. Restored all 1.
- Fixed foveation temporarily off: owner still described the different square
  patches. Restored 1. The later capture was after its automatic restore, so do
  not describe the screenshots as foveation off.
- Owner switched DLSS off in game at the same distance: **still present**.
- Temporarily suppressed bloom upsample pipeline `25811850` in the then-running
  process (PS hash `ffda83db6037f452`): **still visible**. Restored skip_pipeline=0
  after the response; watchdog finished. This reduced soft glow, not all bloom.

Stereo pair at **22:16:14** (left/right PNGs in the game directory) and a native
GPU trace at **22:16:24** capture the affected distance. Trace and 23 decoded
render targets are in ignored `build-frame/lighting-capture-20261008-221614/`;
raw targets remain beside the INI. Targets are overwritten at successive uses,
so one final resource dump does not establish which draw introduced an artifact.
The trace records command-list recording order, not global GPU execution order.
The centered-projection experiment below is installed and passed the owner's
initial waypoint check; wider validation is still needed.

Read-only native inspection found `LightBinPass::Render` at RVA `0xFEA150`,
called by lighting task `0xE02920`; the light bins use a 32x20 grid. This is a
candidate for square lighting artifacts, **not a confirmed cause**. Renderer
singleton is `*(base + 0x34F6480)`; its LightBinPass is +0x1BD58, its clip-plane
buffer member +0x78, and global-frame constant-buffer CPU pointer +0x11BB8
(size 0x5A0). Setup at 0xFE9760, compute shaders `light_bin_c` and
`light_bin_depth_bounds_c`. Global constants built at 0xE3B590. Local assembly
notes are ignored `build-frame/lighting-*.asm`; do not publish raw game code.

Further read-only shader inspection found a concrete projection incompatibility:
`light_bin_c` reconstructs tile rays using only inverse-projection diagonal
CB3[17].x / CB3[18].y, and projects light bounds using CB3[13].x / CB3[14].y.
It never includes the off-axis projection terms. Frame's captured FOVs have
substantial horizontal and vertical asymmetry. Local-only disassemblies are
`build-frame/native-light-bin.asm` and `native-light-bounds.asm`. Shader asset
handles are LightBinPass +0x88/+0x98; object at asset +0x40, DXBC pointer at object
+0x38. This is evidence of an incompatibility, **not yet proof of the visual cause**.

A live `[render] symmetric_projection=1` mode is installed and now the default.
Once per successful XR LocateViews, it encloses both eyes'
recommended FOVs in a common centered frustum. It leaves poses/IPD/head pivot
unchanged and uses that same FOV for projection, VRS center, captures and
xrEndFrame submission. OpenXR supports submitted FOVs differing from the
recommended ones. At equal texture dimensions it loses some angular resolution;
it is a compatibility test, not a performance optimization. Revert live with 0.
Host math tests cover actual Frame FOV containment, diagonal-only ray
reconstruction, compositor mapping and stereo world-anchor convergence under
both projection modes. Source/installer defaults are 1 following the owner's
successful waypoint and subsequent creature-label checks; other headsets remain untested.
Cross build and host math tests passed. Installed after Save & Exit at
**22:42:35**, then relaunched. Backup:
`build-frame/backups/centered-lighting-20261008-224235`. DLL SHA256:
`1dccb3ff0f3e8e8337d91b8e3fc5689668a7c8e9e1ef4701663db46166b79806`.
Only the DLL and this one INI setting changed; Balanced DLSS and prior diagnostics
defaults were preserved. The live INI has legacy malformed header/comment lines,
so strict Python ConfigParser cannot read it; use section-scoped minimal edits.
Runtime **22:42:54.681** confirms symmetric FOV `[-1.03239, +1.03239,
+1.04476, -1.04476]`. Offline gameplay resumed at 22:43:03; both DLSS eye
corrections submitted successfully and the correction shader compiled in 35 ms.
The eye projection log has proj[8]=0 as intended.
Stereo capture **22:44:18** (Frame-pair-1 left/right PNGs in game directory)
confirms both eyes use the centered FOV with distinct positions about 6 cm apart.
The individual images show a coherent first-person scene facing the waypoint;
they cannot establish whether the moving-light artifact is gone. Capture request
reset to 0. The owner's subsequent response was **“patches seem to be gone.
Though the image is a bit softer.”** No new stereo/camera failure was reported.
Keep centered projection enabled locally. Do not compensate by increasing
render resolution or DLSS quality without considering the performance priority.
A more targeted light-bin correction could retain asymmetric projection and
pixel density, but has not been implemented or validated.
The subsequent DLSS-off visual check was much clearer. Recent unprofiled samples
at 22:50:03–22:50:33 measured 30.0–36.7 pairs/s and 17.0–21.2 ms for both eye
draws combined. These were moving/unmatched scenes, not a DLSS performance A/B;
do not infer a percentage performance cost from them. No graphics setting was
changed by the agent after the owner's DLSS-off choice.

### Standby/reconnect crash after lighting trial (parked by owner)

After Frame woke/reconnected at **22:45:24**, the kernel logged NVIDIA **Xid 32**
on RTX 5090 PCI 0c:00 in `vrserver` at **22:45:25**. SteamVR then failed to acquire
backbuffers; D2R's XR submit blocked for roughly 10 seconds repeatedly. SteamVR's
connection watchdog aborted at **22:46:06**, followed by a compositor segfault
at 22:46:07. D2R's 22:46:08 shutdown access violation at module RVA 0x530ED
matches its pre-existing 20:33:23 report. This establishes ordering, not the
ultimate cause or proof that the new projection is unrelated. NVIDIA describes
Xid 32 as a command-stream/DMA fault; do not diagnose hardware failure from this
single event: https://docs.nvidia.com/deploy/xid-errors/analyzing-xid-catalog.html

The owner confirmed it happened after waking/reconnecting, then explicitly said
**ignore this issue for now and focus on visuals**, and requested another launch.
No crash-related code, driver or runtime settings changed. SteamVR restarted
successfully; D2R relaunched with the same lighting fix and Balanced. The XR
session reached focused state at 22:48:57 and stereo gameplay resumed at
22:49:05 with automatic headset calibration. The owner is evaluating visuals.
Evidence preserved locally under
`build-frame/backups/crash-centered-20261008-224642/`: kernel/GPU records,
SteamVR logs, D2R/ReShade/Proton logs and minidumps. Do not upload raw dumps.

`HookDrawGameScreen` holds one headset pose per pair, right_dt_ms=0.01 preserves
particles. Native frame counters/temporal histories and camera-derived lighting
constants still deserve investigation. Do not zero dt or force identical
view-dependent highlights. The negative sky/fog test makes ReShade Timer changes
a poor first candidate. Original sync stays defer_gpu_wait=0; camera vr_mode=4.

Owner additionally reports a fuzzy, ghosted, vibrating desktop mirror. Each
native Present alternates eyes; this can explain mirror vibration/doubling but
not automatically all fuzziness. Individual eye captures at **22:33:06** do not
show large duplicated scene edges; a partially open/transitioning UI is present.
One still pair cannot rule out temporal reconstruction artifacts.

At 21:33:34 another 614 ms cycle contained a **565 ms pre-render gap**. It followed
`InputMethodChanged` and panel-layout changes. This is a clue to UI/input-layout
rebuilding, not proof of cause, and is separate from the usual 80–100 ms eye-draw
spikes. Do not blame shader compilation without more evidence.

Next performance work should attribute engine scene preparation and worker-thread
cost within each eye draw, then investigate reuse of genuinely eye-independent
work. Preserve separate eye cameras, culling, UI and resource lifetimes. Avoid
reducing image quality or removing effects based only on low spot GPU utilization.


The user also reports desktop mirror flicker. `HookDrawGameScreen` runs a full
left/right pair; each native Present displays that eye's backbuffer. The XR
capture is copied separately after effects; there is no third scene render for
the monitor. The ReShade addon `present`/`reshade_present` callbacks return void
and cannot cancel OS presentation. A stable one-eye mirror would require an
extra copy with the current path. No mirror change has been made. Investigate
safe optional display suppression and measure it separately; do not promise
an FPS gain or minimize the game and disrupt focus/input. Recent Present spans
are roughly 0.8–1.1 ms per eye, including more than display-only cost.


Old SDK runtime logging forced synchronous `fsync()` on the game main thread.
Live `/proc/PID/syscall` samples identified syscall 74 and the exact descriptor
path: **`d2rloader/logs/d2r-vr-vrcam.log`**. Several 540–784 ms pauses had repeated
ext4 journal/page waits, no physical disk reads, and almost all time before
stereo drawing. This was our diagnostic logger, not demonstrated asset loading.

`steamvr/logging.cpp` now queues up to 1024 records of 640 characters and writes
worker batches about every 250 ms, ordinary append, **no forced flushes**. Runtime
logs moved to `d2r-vr-frame.log`; rare SDK startup messages remain in the old file.
Queue overflow drops diagnostic messages instead of blocking gameplay on disk.
Concurrent producer/order/drain test passes under installed Proton.

After deployment the user confirmed improved hitches. 599 main-thread samples
showed no journal/page waits or blocked log file paths. Walking timing windows
had maxima around 38–60 ms; one **475 ms** cycle remained, about 445 ms before
stereo draws. Frame-pair throughput varies by scene (roughly 30–53 pairs/s in
observed windows), not a controlled benchmark or headset refresh rate.

Quiet Proton profile retains Wine errors and VKD3D warnings while dropping
exception/unwind tracing (previously >38,000 lines/second). Do not clear shader
caches. Existing VKD3D cache had about 1820 entries plus additions. Most recorded
hitches had no tracked pipeline creation, but deferred driver/compiler work can
escape those callbacks. Shader compilation remains a candidate, not proven cause.
Native height fog avoids repeated mod fog scans; video preset uses 80% resolution
and medium AO/shadows/atmospherics. ReShade sky/fog compilation was subsequently resolved with the private compiler below.

The 19:49 walking test measured about 42–45 stereo pairs/s in several windows,
with the drawing thread using CPU for 94–97% of its render-pass wall time.
RTX 5090 utilization had a median near 43%. Scene variation prevents claiming a
precise percentage gain; user feedback was “Smoother”. A remaining 504.7 ms cycle
spent 474.1 ms before stereo drawing, concurrent with roughly 27 MB of cached
file reads and a busy main thread. Asset processing is suspected, not proven.
Physical disk reads did not increase in the sampled interval.

Local evidence: `build-frame/foveation-cpu-process-samples.csv`,
`build-frame/foveation-cpu-gpu.csv`, backup
`build-frame/backups/foveation-cpu-20261008-194900`. A continuously busy worker
and ReShade's persistent “Compiling (1 effects remaining)” prompted compiler
investigation. An optional compiler can be preloaded from
`GAME/d2rloader/runtime/d3dcompiler_47.dll` before ReShade, only under the mod.
Do not use the old Windows 8.1/Winetricks compiler: it rejects `ps_5_1`/`vs_5_1`.
The Microsoft Windows SDK 10.0.26100.9169 x64 redist compiler passes a `ps_5_1`
probe on Proton. The full effect compiled in 10.56 seconds; subsequent samples
showed no continuously busy compiler worker. Sky rendering now works. This
adds previously missing effects, so do not treat it as a quality-matched FPS test.
Unfilled ceiling texture slots now use an empty source; the existing procedural
stone fallback remains and ReShade no longer reports missing optional textures.
The prefix's system compiler and stock executable remain unchanged.

## Foveation status

Fixed foveation is active: Tier 2 VRS, per-eye asymmetric centers, protected 1x1
ellipse and 2x2 peripheral **opaque** geometry shading. UI, transparency, effects,
shadow-only and unsupported passes stay full quality. Immutable masks are
uploaded once and cached; draw-path lookup uses an epoch to avoid repeated work.
Observed device: Tier 2, 16-pixel tiles; logs confirm reduced-shading draw counts.
User first comparison felt “about the same”; **no measured GPU speedup** claimed.
`[render] fixed_foveation=0` disables; `foveation_inner=0.55` default (0.35–0.85).
No gaze sampling or gaze-driven VRS implemented. Quest 3 does not require gaze
for this fixed technique. Frame eye-tracked rendering remains a later task.

## Build and verification

Clang 21 with Windows SEH support, MinGW headers/static libraries, CMake 3.28+,
Ninja, OpenXR SDK 1.1.62 and D2RLoader Plugin SDK. See steamvr/README.md for commands.
SDK revisions used:

- D2RLoader PluginSDK: `717f727a0ec52912d1558764345f8fa3453a2bd6`.
- Khronos OpenXR-SDK: `57af7fc61f9f2d492580cb28aab6d0ea59d8d417`.

Run native camera math and deferred-draw tests, `python3 steamvr/session_test.py`,
`python3 steamvr/install_test.py`, shell syntax and diff checks. Windows executable
tests for checked memory reads/writes and buffered logging run through Proton.
The checkpoint verification passed those tests, the 17-call NGX ABI comparison
under both MSVC and MinGW code generation, the profile report on recorded frames,
the cross build and shell/diff checks. The rebuilt DLL with defaults matching
the tested settings has SHA256
`02ed0a87c998cba34303027488672c417ca15746a22be491e07effe8ae2e34c0`.
The running game still uses the 23:03 DLL with the same settings explicitly set;
it was not interrupted just to change defaults.
The Windows OpenXR loader is cross-built via `cmake/openxr-loader` with
`cmake/openxr-loader-toolchain.cmake` and packaged with its license notices.
Loader build/export/dependency checks do not establish native Windows gameplay.

## Owner-machine paths and diagnostics

These are local investigation references; friend setup must use its own paths.

- Checkout: `/home/berkybear/diablo2-vr-port`, branch `vr/steam-frame-proton`.
- Steam: `~/.local/share/Steam`, app **2536520**.
- Game: `steamapps/common/Diablo II Resurrected`; prefix:
  `steamapps/compatdata/2536520/pfx`.
- Dependencies: `~/vrdeps`; compiler:
  `~/Android/Sdk/ndk/29.0.14206865/toolchains/llvm/prebuilt/linux-x86_64/bin`.
- Initial complete install/save backup: `build-frame/backups/20261008-171211`.
- Latest pre-deployment snapshot:
  `build-frame/backups/label-order-signature-20261008-230315`.
- Logs: `GAME/d2rloader/logs/d2r-vr-frame.log`, startup `d2r-vr-vrcam.log`,
  `GAME/ReShade.log`, `build-frame/steam-2536520.log` and routing log.
- Local-only profiling: `build-frame/walking-process-samples.csv`,
  `blocking-file-operations.jsonl`, `buffered-logging-process-samples.csv`.
- Game-memory snapshots, saves, account data, logs, builds and captures are
  excluded from git and the release. Do not publish the local game-code dump.

For desktop captures find the current game window with xdotool; IDs/PIDs change.
Frame capture helper in the separate DevilutionX checkout returned a plain blue
image during the latest PCVR session, so do not claim it captured these labels.
Read `/proc` for diagnostic sampling; kernel perf policy blocks normal perf.
Avoid global kernel changes and intrusive tracing while measuring frame times.

## Friend agent's runtime checklist

Read the current WiVRn dashboard setup and compose any required environment
prefix with our Steam wrapper, retaining one `%command%` and the same prefix.
Check a stock OpenXR game first, then our `openxr-probe.exe` / native logs.
The transport requires **XR_KHR_D3D12_enable**, translated by WineOpenXR under
Proton. WiVRn being OpenXR-compatible alone does not prove the game binding works.
Do not switch the owner's active runtime or start another headset stream during
an existing Frame test. Keep Windows launcher and Quest controller mappings
marked untested until verified. No friend invitation is sent without a username.

## Owner headset recording tool

Framecorder 0.1.1 from https://framecorder.coah80.com/ is now installed on the
owner's Frame, using its checksum-verified released ARM64 package. Dashboard,
sync service and automatic update timer are active. The SteamVR dashboard's
Framecorder tab provides Record/Stop; saved videos go to `~/Videos/framecorder`.
Manual recording is configured, game audio on, microphone and instant replay off
while diagnosing D2R performance. Left-stick replay clipping is therefore off.
A four-second headset-source recording was verified: 1920x1080 HEVC, 90 fps,
361 frames and visible passthrough imagery. Temporary test files were removed.

It currently uses SteamVR's headset-view fallback. Direct panel capture needs a
one-time administrator password entered locally on Frame (sudo is not passwordless):
`sudo -v && ~/.local/bin/framecorder-setup --unlock`. Do not request/store the
password in chat or restart the tab during an active recording. The official
installer also exposes the unlock step. No Framecorder source was modified.
