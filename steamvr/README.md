# Native OpenXR PCVR — technical notes

For the compiled preview and portable friend setup, start with
[INSTALL.md](../INSTALL.md). These notes retain the owner machine's build and
Steam Frame testing details. Quest 3/WiVRn/Windows paths remain untested.

This branch adds a native OpenXR transport to the MIT-licensed
[D2R VR camera mod](https://github.com/natpoh/diablo2-vr-port). It reuses its
first-person camera, body, controller aim, and two scene draws per game update.
The new transport submits those separate eye images to SteamVR through Proton's
WineOpenXR D3D12 bridge. BodyWalk and FlatVR are not required by this build.

## Current verification

- Cross-compiles with Clang 21 and MinGW headers/static libraries.
- Camera math checks cover asymmetric eye FOV, physical eye separation, view
  orientation, and the game's reverse-Z projection.
- The plugin's camera, input and stereo hooks load with Steam D2R **3.3.93787**.
- Fixed MinGW/MSVC aggregate-return ABI differences in the vendored ReShade
  interfaces; the title screen now runs without the earlier callback crashes.
- The independent `openxr-probe.exe` successfully negotiates WineOpenXR,
  creates a D3D12-enabled instance, identifies `SteamVR/OpenXR`, finds Frame,
  and creates a matching D3D12 device, queue, and OpenXR session.
- The offline character screen was reached twice with Proton's `S:` game-drive
  mapping. The warning returned on a launch where that mapping was absent;
  the wrapper now explicitly supplies the library/install paths to keep it stable.
- Game-owned D3D12 session/swapchain creation and image copies succeed. SteamVR
  exposes an SRGB format for this SDR backbuffer; the transport selects that
  compatible format while preserving the game image bytes. A headset capture
  confirms the title screen on a world-anchored panel.
- Offline first-person gameplay is running on Frame. The user confirmed it works;
  a headset capture shows Nekro's hands and weapon in the Rogue Encampment.
- Height calibration checks cover seated height, continued crouching, unchanged
  horizontal position and eye separation, and keeping calibration clicks out of
  both the map action and the emulated gamepad.
- A crash in `probe::SafeCopy` during the background hero-facing scan was traced
  to an inaccessible page. The native branch uses checked `ReadProcessMemory`
  reads there; its Proton test rejects inaccessible, freed and null memory.
- Menu hit tests cover panel edges, misses, rear-facing rays and rotated panels.
- A second scan crash was traced to `gamefog::ScanThread`. The native scan now
  searches bounded snapshots and uses checked writes, rather than walking live
  pages directly. Checked reads and writes pass the Proton memory test.
- Player-class skeletons are filtered by camera-target proximity and original
  standing head height before they can control the hands, facing or body. The
  corpse-at-respawn case is included in the selection checks. This is a spatial
  and pose filter, not an authoritative unit-ID binding.
- Launcher checks cover ordinary exit codes, cleaning helpers after game exit,
  and leaving unrelated processes running.

## Run

Keep Steam and SteamVR running, with Frame connected for PC streaming.
Set this game's Steam Properties → General → Launch Options to:

```text
"/home/berkybear/diablo2-vr-port/steamvr/launch.sh" %command%
```

The normal Steam **Play** button still starts the original, unmodded `D2R.exe`.
To explicitly launch the offline VR prototype:

```sh
./steamvr/run-proton.sh
```

This runs `steam -applaunch 2536520 --frame-vr`. The wrapper consumes that
argument, replaces only the game executable with `D2RLoader.exe`, and preserves
Steam's runtime, Proton, prefix, launch arguments, and authentication context.
For VR it also imports the host OpenXR runtime into the Steam Linux Runtime.
Use the offline character **Nekro** for game testing.

Main menus and in-game panels use a world-anchored display. Aim the right
controller's cyan laser, use **right trigger** to click or drag, and **right B**
to back out. **Keep the D2R window focused on the PC.** Mouse input is sent only
while that window has focus; after using another desktop app, select D2R again
before clicking or dragging in menus. Opening inventory with Left X does not
guarantee desktop focus. Gameplay
uses separate projection views with the headset's
actual eye positions and FOV. Controller actions feed the existing game-side
input bridge. Native VR stays in first person and F11 recenters the camera.
The legacy F1–F3/F12 view shortcuts cannot switch native VR to an incompatible
camera. Old saved VR modes are repaired to first person on load. Flat monitor
mode and the separate FlatVR build retain their view selection.

The desktop game window currently presents the alternating eye frames, so it
can visibly flicker even when the headset stereo is correct. It does not render
a third scene for the monitor. Desktop presentation still has overhead;
suppressing it safely is an open performance task. Do not minimize the game as
a workaround: background/focus handling also affects input and frame pacing.

**Left X** opens inventory through the game's UI service when no menu is open.
When menu panels are open, Left X closes them, including panels opened with the
toolbar shortcuts; another press opens inventory again. It preserves the HUD
and map. The native inventory action requires headset focus, so another active
desktop window does not swallow Left X. Explicit menu actions and laser clicks
try to bring the game window forward; OS mouse/keyboard injection still requires
desktop focus. Hovering the laser does not move focus. The controller menu button
and right B in menus send Escape using a
scan-code press held across frames. A trigger used on a menu remains consumed
until released, so closing the panel cannot turn the same press into an attack.

Hold **left grip**, then **click the left stick** to reset eye height to the
current headset position. Use it after sitting down or standing up. It preserves
heading and horizontal room movement; ordinary left-stick clicks still toggle
the map. Release the stick before another reset. F11 also resets height as part
of its full camera recenter, including horizontal position and facing.

Each saved-game load calibrates from the current headset position at the first
tracked, focused game frame, then recenters facing. Movement during launch is
not carried into the player's body offset. Opening and closing a dialogue or
inventory does not repeat this calibration. Dialogue panels disable the game's
eye alternation and retain its original camera/projection, giving both eyes the
same stable image on the panel.

After the game process exits, the VR launcher gives Proton three seconds to
finish, then closes helpers belonging to that launch if they are still keeping
Steam's session open. It does not terminate a running game. Normal Steam Play
still runs without this wrapper's VR session monitor.

The performance preset renders the scene at 80% resolution scale (64% of the
original pixels) and uses medium shadows, ambient occlusion, and atmospherics.
Textures stay at their original quality. Both eyes have their own copy command
list, with one GPU completion wait after the pair instead of between the draws.
Native height fog is retained, avoiding the mod's repeated full-memory scans
after area changes. This is a performance baseline and can change atmosphere.
Hitches over 80 ms log game gap, XR begin/wait, both eye draws, XR end/GPU fence
and pipeline creation timing; menu intervals are excluded from gameplay samples.
Pipeline creation timing is a clue to compilation costs, not a complete shader
profiler: work deferred until a draw or performed outside ReShade callbacks may
not appear in those creation measurements.

Fixed foveation uses D3D12 Tier 2 VRS with a separate mask for each eye's
asymmetric optical center: 1x1 in the protected center, 2x2 in the periphery.
It applies to single-sample opaque world geometry with scene depth, leaving
transparent passes, menus, labels, post effects and shadow-only passes at full
quality. Unsupported devices/command lists fall back to full quality. Immutable
masks are uploaded once per viewport; routine draws do not upload or wait for
them. `[render] fixed_foveation=0` disables it for comparison; `foveation_inner`
sets the protected ellipse radius (default 0.55, range 0.35–0.85). Settings reload
live. Logs report mask creation and reduced-shading draw counts. The user reported
similar perceived performance in the first walking test; this is not a measured
GPU speedup. No gaze data is used yet.

`[hud] stereo_labels=1` projects the game's native unit label anchors through
the current eye's exact scene view and projection, including asymmetric FOV and
physical eye separation. This bypasses the label path's inline flat projection
rebuild and keeps native names, health values and styling. It is scoped to unit
labels; menu UI and shadow fitting retain their original projection behavior.
The native placement also mixed projected foot X with projected head Y. The
updated hook combines the original world coordinates into one head-height anchor
before projection, so pitched/rolled eyes refer to one 3D point. Both the
matrix-only prototype and unified-anchor correction still doubled in the user's
headset tests. Centered projection subsequently improved apparent convergence;
the remaining reversed label disparity was fixed by current-eye draw ordering
below. Disabling the anchor correction alone did not improve depth.
The owner verified the combined fix on Steam Frame. Stereo math checks include
head pitch/roll; bosses and wider scene/headset coverage remain untested.

`[hud] monster_draw_order=1` is enabled by default: paint the
native monster health panel after the current eye's tooltip updates, within the
same UI pass. Hooks are signature-checked on launch; set 0 live
to compare native ordering. Names, health data and native styling are retained.

F8 saves one complete gameplay pair through ReShade, with matching
`Frame-pair-N-left/right` suffixes. This readback briefly stalls: exclude it from
performance measurements. For remote debugging, increment `[debug] capture_pair`
in the installed INI, or set/increment `capture_labels` to wait for a visible
unit label before capturing. Normal play performs no readback or screenshot IO.

The current Proton installer supplies the Microsoft SDK shader compiler privately
under `d2rloader/runtime`. ReShade had remained compiling throughout tests using
Wine's compiler; the SDK version completes the effect. Missing optional ceiling
art uses the procedural fallback. Stock Steam Play does not load this dependency.

## Build

The local dependency checkouts and extracted toolchain are under `~/vrdeps`.
The Android NDK provides Clang's Windows SEH support; it does not make this an
Android or standalone Frame build.

```sh
export MINGW_ROOT=/home/berkybear/vrdeps/mingw/usr
export CLANG_ROOT=/home/berkybear/Android/Sdk/ndk/29.0.14206865/toolchains/llvm/prebuilt/linux-x86_64/bin
export D2RL_SDK=/home/berkybear/vrdeps/D2RLoader-PluginSDK
export OPENXR_SDK=/home/berkybear/vrdeps/OpenXR-SDK
cmake -S steamvr -B build-frame -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/proton-toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-frame -j4
g++ -std=c++20 -Wall -Wextra -Werror steamvr/math_test.cpp -o /tmp/d2r-frame-math-test
/tmp/d2r-frame-math-test
g++ -std=c++20 -Wall -Wextra -Werror steamvr/deferred_draw_test.cpp -o /tmp/d2r-frame-ui-test
/tmp/d2r-frame-ui-test
python3 steamvr/session_test.py
python3 steamvr/install_test.py
python3 steamvr/ngx_abi_test.py --clang "$CLANG_ROOT/clang++"
```

To build the additional native Windows OpenXR loader for a preview package:

```sh
cmake -S cmake/openxr-loader -B build-frame/windows-loader -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/openxr-loader-toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-frame/windows-loader -j4
# Commit source first; the package records that commit and file checksums.
python3 steamvr/package.py \
  --openxr-loader build-frame/windows-loader/sdk/src/loader/openxr_loader.dll
```

The install script verifies the game version and the official D2RLoader archive
checksum, backs up saves and any replaced files, installs the plugin and ReShade
assets, and adjusts video settings. It never replaces the original `D2R.exe` or
installs an automatically loaded DXGI proxy.

```sh
python3 steamvr/install.py
```

The initial save and installation snapshot is:
`build-frame/backups/20261008-171211`.
To remove this installed prototype and restore its original video settings:

```sh
python3 steamvr/install.py --restore build-frame/backups/20261008-171211
```

Restore preserves current character progress. Clear Steam's launch options if
removing the wrapper or moving this checkout. Character files can be restored
separately from the snapshot's `saves/` directory if needed.

## Diagnostics and remaining work

The prioritized feature backlog and headset verification checklist live in
[TASKS.md](TASKS.md), including UI, combat gestures and eye-tracked foveation.

For a bounded performance capture, change `[debug] profile_capture` to a new
positive integer in the installed `d2r_vr.ini` while gameplay is running. The
next 360 valid stereo pairs record per-eye CPU spans, sampled mod callbacks,
OpenXR wait/locate/input/acquire/copy/fence/release/submit spans, and optional
D3D12 timestamps. The probes then become idle automatically. Use a stationary
capture first, then a separate sequence for walking; retain the same scene and
graphics settings for comparisons. Set the counter to zero after testing so
the request does not repeat on the next launch.

```sh
python3 steamvr/profile_report.py "/path/to/game/d2rloader/logs/d2r-vr-frame.log" \
  --sequence 1 --csv build-frame/profile-stationary.csv
```

Callback scopes are inclusive, cover only the main stereo drawing thread, and
high-frequency callbacks are sampled 1/64. GPU scene spans include idle queue
time while CPU commands are prepared; they are not GPU busy-time measurements.
Effects span includes the ReShade overlay. GPU query readback uses the
conservative pair completion fence. Set `[debug] profile_gpu=0` for CPU-only
captures when comparing deferred GPU waits; GPU profiling intentionally waits
for its readback and would hide that optimization. Two timestamp submissions
and scope measurements add diagnostic overhead. Do not compare a profiled
run against an unprofiled run as if all timing changes were optimizations.

`[render] symmetric_projection=1` is the default lighting compatibility
setting. D2R's native light-bin shader assumes a centered projection;
the headset can recommend offset frusta. This mode renders a centered FOV that
encloses both recommended views and submits that exact FOV to OpenXR, preserving
the original eye poses and head tracking. It samples the setting once per pair,
so a live change cannot split the eyes between modes. At the same resolution it
reduces angular pixel density. The owner reports the waypoint patches appear gone
on Steam Frame, with a softer image; other scenes/headsets remain unverified.
Set 0 to restore runtime-recommended projections. See the
[OpenXR projection-view contract](https://registry.khronos.org/OpenXR/specs/1.0/man/html/XrCompositionLayerProjectionView.html).

`[render] defer_gpu_wait=1` is an experimental live comparison switch. Image
copies and state restoration are submitted on the D3D12 queue bound to OpenXR;
completion is checked before each eye's copy allocator is reused and before
shutdown, instead of blocking the CPU before every image release. Set it to 0
for the original conservative behavior. See the
[OpenXR D3D12 queue/state contract](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XR_KHR_D3D12_enable-swapchain-image-state.html)
and [OpenXR implementer guidance](https://community.khronos.org/t/do-you-have-to-do-cpu-sync-before-xrendframe/110117).
Proton's `wineopenxr` also drains the bound command queue before enqueueing its
release layout transitions. The source default remains 0. The owner's repeated
comparison reduced the XR end stage from about 5.7 to 3.8 ms; part of the removed
fence wait moved into runtime submission. Total cycle time also trended lower,
but head direction differed between captures and a competing CPU-heavy test was
running, so this is not a controlled percentage speedup. The owner reported
bursty walking with either setting. It is disabled in the installed configuration
pending further work. See HANDOFF.md for sample details; other headsets/runtimes
are untested.

- Runtime log: `d2rloader/logs/d2r-vr-frame.log` in the game folder. A bounded
  queue writes batches on a worker without forced disk flushes. The old SDK log,
  `d2r-vr-vrcam.log`, remains for loader/startup messages. Live syscall sampling
  identified repeated main-thread `fsync()` waits on that old file during hitches.
  The new logger's concurrent-producer and shutdown-drain test passes in Proton.
  The user confirmed improved hitches after deployment. A subsequent 599-sample
  main-thread trace saw no journal/page waits or captured forced log flushes;
  one 475 ms gameplay gap remained in that walking test.
- ReShade log: `ReShade.log` in the game folder.
- Proton log: `build-frame/steam-2536520.log`.
- VR launches retain Wine errors and VKD3D warnings but disable Proton's default exception
  and unwind traces. Those traces produced over 38,000 lines in one second during
  walking tests. Game-side frame/pipeline timing remains enabled. Set
  `D2R_FRAME_WINEDEBUG` to an explicit Wine debug profile for a targeted diagnostic
  launch; normal Steam Play does not receive the VR logging settings.
- Independent probe result: `build-frame/openxr-probe-result.txt`.
- By default capture waits for both GPU copies before releasing eye images.
  The experimental deferred-wait mode above retains completion checks at
  allocator reuse and teardown instead.
- Measure the updated performance preset and test the height chord on Frame.
- Verify laser selection, inventory dragging, menu placement and longer gameplay
  after the checked-read fix on Frame.
- Verify corpse filtering, calibration after loading, dialogue stability and
  Steam returning to Play after the game's own Exit command on Frame.
- Belt, health and mana still use the original screen HUD. A separate native
  toolbar plane is planned; FlatVR's existing shared HUD is not yet consumed.
- ReShade's sky/fog now compiles using the private modern SDK compiler described
  above. Include its GPU cost when profiling; earlier incomplete-compilation
  sessions did not render the same effects.
- Further checks remain for menus, HUD layout, combat and longer play sessions.

Upstream provenance: `natpoh/diablo2-vr-port` at
`10cfb0a2a9080184101794d3ac8c238c35717967`; official D2RLoader 1.3.1-beta and
ReShade 6.8.0 with full add-on support. Existing copyright and license notices
are retained. The original `emmericp/D2R-3D` checkout remains separate.
