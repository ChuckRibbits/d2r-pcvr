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

Latest installed build: buffered logger + unified creature-label anchor,
launched approximately 19:23 local on 2026-10-08. User response:
**“Hitches improved, labels still doubled.”** User then requested pushing all
work to a private `bearstonem` GitHub repository, a usable compiled preview,
quick installation instructions for the friend, and this handoff document.

1. **Fix creature names/health stereo.** Two prototypes failed visual testing.
   Labels for normal creatures include health in the same floating name label;
   bosses are not yet checked. Correct position/projection alone has not fixed it.
2. **Residual performance hitches.** A proven mod-induced disk stall was removed;
   remaining stalls need attribution, not blanket shader-compilation claims.
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

Next capture **a single-eye backbuffer** and inspect whether labels are duplicated
within one image, or instead mis-converge between eyes. Investigate cached UI
layout, eye/frame pairing, and label rectangle/glyph placement as a billboard.
UnitToScreen callers found: 0xCE877, 0x198E90, 0x198FCE, 0x1FABDF (items),
0x6D7B15, 0x1519482, 0x1519875. Helper 0xCE850 applies scaled UI offsets.
The ReShade UI trace (`vr/uitrace.cpp`, Ctrl+F10) can dump one GPU frame and raw
targets. It causes a capture pause; do not mix it into a performance comparison.
UI target is R16G16B16A16_FLOAT with **inverse alpha**, cleared black/alpha 1.
Do not infer duplication merely from different left/right pixel positions:
the headset's asymmetric optical centers produce large legitimate differences.

## Performance findings

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
and medium AO/shadows/atmospherics. ReShade sky/fog compilation remains unresolved.

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

Run native math tests, `python3 steamvr/session_test.py`,
`python3 steamvr/install_test.py`, shell syntax and diff checks. Windows executable
tests for checked memory reads/writes and buffered logging run through Proton.
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
  `build-frame/backups/buffered-logging-label-anchor-20261008-192323`.
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
