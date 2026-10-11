# Clean Windows installation and agent handoff — October 10, 2026

Start here for the owner's **fresh Windows installation**, using Steam Frame
over SteamVR PC streaming. This is a native OpenXR mod of the **Steam edition
of Diablo II: Resurrected 3.3.93787**, for **offline characters only**.
Native Windows setup passes the SteamVR/OpenXR D3D12 session probe and native
build checks. The owner confirmed offline VR gameplay works and the native
performance candidate runs "superbly". A follow-up build addressing rendering
and transition bugs is installed; belt rendering, cave wrist values and the
camera clipping correction are owner-confirmed. The owner reports "it all works"
after the cave melee coordinate fix and offline retest. The original
Linux evidence remains separate.

**October 11 cave follow-up on `fix/cave-melee-recovery`:** PR #2 is merged;
local main was rebuilt and installed as `95806d91...`. The 12:16 SteamVR
shortcut session entered `act1_caves` at 12:20:36 with wheel selection 1,
which disabled contact melee. After returning to Weapon mode and reentering
at 12:21:26, logs show repeated recovery with no sweep requests. This differs
from the earlier grid-shear rejection. Session logs are preserved locally at
`build-frame/windows/cave-melee-regression-20261011-122704/`.

The input engine still had a 35 rad/s cutoff, while posed contact recognition
already accepted up to 60 rad/s. An integrated synthetic regression reproduced
fast swings being cancelled upstream; both now share the 60 rad/s limit.
At the owner's request, free melee also remains available with a wheel skill
selected. Casting, menus and potion handling still pause contacts; trigger and
quick-cast bindings remain. Checks cover repeated area loads, selected skills,
36/44/72/90/120 Hz sampling and actual tracking-jump rejection. All 34 native
tests pass. Candidate `d68f861e...` was installed at 12:30 PDT with INI,
calibration and graphics unchanged. The previous DLL and saves are backed up at
`build-frame/backups/windows-melee-recovery-20261011-123042/`; deployment record:
`build-frame/windows/latest-melee-recovery-deployment.json`. SteamVR relaunch
at 12:30:57 reached outdoor gameplay and entered the cave at 12:38:59. Logs
confirm 36 accepted outdoor and 72 accepted cave melee contacts with no
position mismatches. Owner feedback and selected-skill/repeated-transition
confirmation remain pending. The session returned to the frontend at 12:45:34,
then requested exit; D2RCore unloaded at 12:45:40. Shutdown emitted native
pool assertions, so do not describe teardown as assertion-free. D2R is closed.
Retest evidence:
`build-frame/windows/melee-recovery-session-summary.json` and matching session
log. The original cave coordinate and native wall/warp checks remain intact.
The v0.3.1 Windows preview includes these changes and player-facing Steam
library shortcut directions. Release evidence:
`steamvr/evidence/2026-10-11-windows-release.json`.

**Quest follow-up on `vr/quest-2-3-support`:** PR #1 is merged. Quest 3 with
Steam Link/SteamVR is owner-confirmed using `f559b3e5...`. Virtual Desktop/VDXR
reaches tracked gameplay, but its initial run has washed-out colors from
submitting copied sRGB bytes as linear UNORM. Candidate **`109dfa0e...`** is
installed at 18:35 PDT and selects sRGB even when UNORM is also supported.
All **34 native tests** pass, including a GPU regression that reproduces the
old gamma error; typeless sRGB potion stereo/readback passes. The previous DLL
is backed up and INI/graphics hashes are preserved. The owner confirms normal
VDXR colors; outdoor melee contacts and normal exit at 18:46 are logged.
Release `v0.3.0-windows-quest-preview` adds Journal wheel-entry removal in
**`e12a5e95...`**, preserving access through the native tabbed menu. All 34 native
tests and the generated wheel preview pass; that final cleanup has no additional
headset session. See [INSTALL-WINDOWS.md](INSTALL-WINDOWS.md) for the user guide
and [steamvr/QUEST_TESTING.md](steamvr/QUEST_TESTING.md) for current
coverage and local deployment record
`build-frame/windows/latest-vdxr-color-deployment.json`. Current launcher routing
passes **24 Python tests** and keeps normal Steam Play stock.

## Windows setup completed October 10

- Steam game build **3.3.93787** is installed at
  `C:\steam\steamapps\common\Diablo II Resurrected`.
- The archived baseline **16e24f55…** passed first offline headset startup, with fresh 80%/medium
  graphics defaults, fixed foveation on, gaze foveation off and GPU wait 0.
  It froze after opening Akara dialogue. Its DLL/logs and a thread dump are
  preserved locally. The native performance candidate **9b3ce9df…** was
  installed with the same settings and a deferred window-focus fix; see below.
  That session logged Akara opening/closing twice and ended with normal Save &
  Exit. The last owner-verified Steam Frame build is **f559b3e5…**, described
  below; the current Quest color candidate is listed above.
- Stock D2R and the Steam Frame dashboard were confirmed working by the owner.
  All mod gameplay/testing is **local offline characters only**. Normal Steam
  Play routes to stock D2R; `--frame-vr` explicitly selects the mod.
- Native SteamVR/OpenXR probe created a D3D12 device, queue and session on the
  **RTX 5090**, with recommended eye images **2160×2160**. The actual game log
  reports **3840×2160 per eye**, derived from its desktop backbuffer. The probe
  recommendation is not the game's eye size. An RTX 4090 is also
  installed; verify the game uses the same adapter when profiling.
- Windows' system OpenXR default was Virtual Desktop. The Steam wrapper's
  `--frame-openxr-runtime=PATH` option selects SteamVR **only for VR launches**
  through `XR_RUNTIME_JSON`. Frame uses SteamVR streaming. No Virtual Desktop
  runtime is needed for this test path.
- Portable Python, CMake, Ninja, LLVM-MinGW and both pinned SDKs are under
  `deps/`. Native compilation, **33 C++ tests**, **21 Windows Python tests** and
  the **17-call NGX ABI check** pass. Linux process-wrapper tests are not part
  of the Windows suite. Native DLL imports require only Windows/UCRT libraries.
- Defender quarantined the checksum-matched official D2RLoader dependency as
  `Trojan:Win32/Suschil!rfn`. The owner explicitly authorized exact loader-file
  exclusions. The applied additions and previous settings are recorded locally
  in `build-frame/windows/defender-loader-exclusions.json`; global protection
  was not disabled. Matching a pinned hash does not prove a detection false.
- Save/file backup: `build-frame/backups/20261010-163404-084243`.
  Native probes, launch options, stream logs and setup evidence are under
  `build-frame/windows/`. These local files and saves remain excluded from Git.

The launch option copied into Steam on this machine is:

```text
"C:\Users\berke\d2r-pcvr\deps\python\python.exe" "C:\Users\berke\d2r-pcvr\steamvr\launch_windows.py" "--frame-openxr-runtime=C:\steam\steamapps\common\SteamVR\steamxr_win64.json" %command%
```

Keep the checkout at this path while that launch option is configured. Local
`build-frame/windows/Launch D2R VR.cmd` and `Launch Stock D2R.cmd` start the
corresponding Steam routes. Save & Exit before changing DLLs. The first baseline
startup is owner-verified; a bounded native performance comparison is pending.

### Current verified cave melee build (installed 17:55 PDT)

The owner entered the cave in the installed calibration build after the 17:49
SteamVR launch. Its logs reproduce energetic swings rejected by the position
guard. Twelve paired read-only samples are saved locally at
`build-frame/windows/cave-positions-1791679988.json`. Native path coordinates
are flat; the renderer offsets both X/Z by terrain height times sqrt(3/2).
The cave floor is about 3.03 world units high, adding about 3.71 world units
to each axis. Removing that shear leaves only about 0.02 world units per axis
against the client; the current server also matches within ordinary movement
lag. This explains the previous approximately 1.35 m false warp rejection.

Candidate `f559b3e57d73cb38e9091753782e7702a53b5185db5108b5a8c488011ea1006c`
is installed. `steamvr/world_coordinates.h` converts the saved rendered
player pose back to the grid plane for the unchanged 0.65 m guard. Enemy contact
capsules receive the forward conversion at the same floor height. No offset is
learned from a newer player position; old swings still reject after a teleport.
Native target, wall, session, freshness and per-stroke checks remain. All 33
native tests pass, including captured cave rejections, existing overworld
positions, negative/elevated floors, scale changes, actual swept contacts/misses
and warp rejection. Build log: `build-frame/windows/cave-coordinates-build.log`.
Normal Save & Exit/full closure completed at 17:55, followed by installation at
17:55:16 and a SteamVR offline relaunch request. Previous DLL, INI, logs and the
current saves are backed up at
`build-frame/backups/windows-cave-coordinates-20261010-175516/`.
Record: `build-frame/windows/latest-cave-coordinates-deployment.json`.
Calibration/INI and graphics hashes were preserved across deployment. The retest
started at 17:56, entered `act1_caves` at 17:58:16 and returned outdoors at
18:02:15. Cave logs show repeated accepted native melee contacts with
`position_mismatch=0`; the owner reports **"it all works"**. Save & Exit and full
closure followed, with D2RCore unloading at 18:02:34. The game is now closed.
The current native build passes 33 tests, 21 Windows Python tests and all 17 NGX
ABI calls under both GNU and MSVC targets. Controlled performance comparison
remains pending; the owner confirmation does not measure an FPS gain.

### Earlier body calibration and shoulder swap build (17:43 PDT)

Earlier installed DLL:
`a1e2006a6efc5bc413ee1241b00a7a7da94a42191eb0ad47bf8fc3bb74b60345`.
The owner confirms calibration saves and closes, and reopening it from the VR
settings menu works as intended, but initially saw a SteamVR
floor/setup message despite an existing playspace. The pose check incorrectly
reported a short resting controller span as a floor failure. This build separates
tracking, missing floor space, standing eye height and arm-pose feedback; normal
resting arms now request a T-pose. Regression coverage includes resting arms on
first load followed by a successful capture with the same stage floor. All 32
native tests pass. Installed at 17:43 PDT after normal game exit at 17:41;
backup `build-frame/backups/windows-calibration-20261010-174350/`, record
`build-frame/windows/latest-calibration-feedback-deployment.json`.
Saved calibration remains eye height 1.64763 m / controller span 1.50561 m from
the owner's second capture. The correction has not yet been retested in-headset.
The game remains closed, ready for the next SteamVR offline launch.

Initial calibration DLL: `e27084edeee20e2b74897b0d70a1e5e186cc30cda4a4d70d0161750dac2647ad`.
The game was fully closed before replacement; current saves, the previous DLL
and INI are backed up at `build-frame/backups/windows-calibration-20261010-173910/`.
Deployment: `build-frame/windows/latest-calibration-deployment.json`.
The existing corrected camera INI and graphics hashes were preserved during
deployment. SteamVR offline launch was requested after installation.

User requested first-load T-pose calibration, repeatable from VR settings, and
dominant-trigger weapon swapping over either shoulder instead of hip anchors.
These are implemented: **Left Grip + Y → Calibrate body** starts the same flow
as first load. Stable upright stage-floor eye height and grip span are saved
atomically, then applied to world scale and character arm proportions. Controller
motion remains physical 1:1. Invalid tracking/posture/floor samples do not save;
cancel, retry and save/conflict feedback are included. Held inputs remain blocked
until release. VR → Dominant hand chooses the shoulder swap trigger (Right by
default); the native equipment request waits for the observed opposite set.
Holding grip does not prevent repeated deliberate swaps. Hip anchors are absent
and their old INI keys no longer enable them. See `steamvr/BODY_CALIBRATION.md`.

**32 native tests and five installer tests pass**, including real Win32 profile
transactions, calibration failure/conflict acknowledgements, arm fitting,
both dominant hands/shoulders and native set confirmation. Rendered calibration
and settings previews were inspected. Live logs confirm the first-load prompt,
opening calibration from VR settings, a successful save at 17:41:02 PDT
(eye height 1.65854 m, controller span 1.50859 m), and released-input return to
gameplay. The `[calibration]` values were verified in the installed INI. Owner
confirmation of the settings-menu calibration flow is complete. Body fit and
shoulder swapping still await owner feedback.
Cave swings remain unresolved: this build
logs server-grid and render-player coordinates on mismatch while retaining the
0.65 m warp guard. The terrain-coordinate hypothesis needs cave evidence before
changing collision conversion. No native baseline/candidate timing claim.

### Earlier rendering and dungeon-transition follow-up

Owner-tested follow-up `996cc430…`: belt potions now render, but sit about a foot
too far forward; torso remains invisible and cave swing attacks still stop.
At 17:08:14 PDT the cave transition reproduced fresh UI snapshots with
`vitals=0`, native vitals reason 4 (expired local identity), while the UI queue
continued to execute. The callback queue was not the cause of that wrist failure.

Current build installed at 17:14 PDT:
`5847a204e994735b656d003944714341c7443d133b2b8176a1e43b484cfe90eb`.
It refreshes native local identity from the signature-checked UI-thread player
accessor after the SDK confirms the current session. Melee/bow read a fresh,
revalidated native unit from that source rather than the camera/aim proximity
heuristic. Server position, target, freshness and wall checks remain in place.
30 native tests pass, including the actual combat identity implementation with
no aim-hook updates, a replacement unit, session changes, reused pointers and
stopped UI reads. Empty equipment no longer repeatedly flips weapon sets after
death. The belt forward clamp now permits 0.03 m; the owner's deployment
set `belt_forward=0.06` (previous default 0.26 m), about eight inches closer.
Graphics are unchanged. Deployment is recorded in
`build-frame/windows/latest-identity-deployment.json`; previous DLL, INI, current
saves and logs are in `build-frame/backups/windows-identity-20261010-171436/`.
The previous session ended with normal Save & Exit. Offline SteamVR relaunch
was requested. The owner now confirms wrist HP/mana remains visible in the cave;
free-swing damage still fails there but works outside. At 17:18:46 PDT the cave
transition retained fresh valid vitals, while energetic melee callbacks ran and
rejected requests with `position_mismatch` (up to 253 per report). Investigate
rendered versus native path coordinates; do not remove the warp/position guard.

Owner supplied 13 F8 stereo pairs at 17:12:08–17:12:39 PDT. They show detached
hands and distorted/clipped torso/skirt geometry. He notes he contorted his
body to reach the keyboard, so do not infer a pose bug from these alone.
Remote normal-posture captures at 17:16 and 17:22 reproduced missing torso and
partial shield geometry in both eyes. Disabling head hiding did not repair it.
The owner described a large invisible clipping volume around his head.

**Torso/arms/shield clipping is owner-confirmed fixed at 17:26 PDT.** The installer
split the default INI at a `[camera]` mention inside an introductory comment,
emitting a malformed first camera section. Win32 profile reads ignored the real
camera section, including `near=0.2`, and used the default 1.5 world-unit near
plane. Removing that malformed prefix live restored the intended values; a
read-only check of the running DLL confirmed near=0.2. Installer parsing now
starts at the complete camera section line, with a regression check through the
actual Win32 profile API (five installer/launch tests pass). No DLL replacement
was needed. Backup and evidence: `build-frame/windows/camera-ini-fix-20261010-1724/`.
Head hiding and body locking were restored after comparisons. Capture counter
is zero. Stereo readbacks briefly stall; exclude them from performance timings.

The owner's current priority is missing torso and belt items, plus loss of
wrist HP/mana and free-swing damage after entering a dungeon. Trigger attacks
continue to work. Hands/weapons and feet are visible; torso diagnosis is open.

Previous follow-up installed at 17:04 PDT (superseded above), DLL SHA256:
`996cc430ca276d65a1aaa877de960b2c95a8dd4ea2c5ae2d1062cdabcfbe134d`.
It retains the native performance changes and focus fix. SteamVR's D3D12 probe
confirmed requested sRGB format 29 returns typeless resource format 27, and
requested float format 10 returns typeless resource format 9. Both potion and
dialogue conversion renderers previously rejected these resources. Explicit
typed views now pass native GPU readback tests for typed/typeless targets,
including potion stereo disparity using SteamVR's sRGB format. All 29 C++ tests
pass. The resource-format failure is reproduced; headset validation is pending.

The loader contract discards queued game work on session changes. Melee and UI
read callbacks now use unique tickets and bounded leases (500/1000 ms), allowing
recovery if work is discarded. A late callback cannot consume a newer batch.
Existing attack freshness, session, position, wall and target checks still apply.
This is a possible cause of the reported transition bugs, not a proven diagnosis.
Low-rate diagnostics now report snapshot freshness/vitals, melee callback and
rejection counts, and body pose geometry. No graphics/camera/INI changes were made.

Deployment: `build-frame/windows/latest-transition-deployment.json`.
Backup of the previous candidate, current saves/INI and logs:
`build-frame/backups/windows-transition-20261010-170426/`.
Probe: `build-frame/windows/swapchain-probe.txt`; build/tests:
`build-frame/windows/transition-build-test.log`. All raw evidence stays local.
An offline SteamVR launch was requested. Next check: belt, enter dungeon,
wrist values and swing damage; F8 looking down records a stereo body image.
Exclude that readback pause from performance measurements.

### Previous native performance candidate and baseline dialogue hang

The owner initially prioritized performance before the missing body; after
testing the performance candidate, he requested the visual/transition fixes above.
The baseline ran well subjectively, then both desktop and headset froze after
Akara opened `NpcDialogPanel` / `ScrollSpeechPanel` at 16:39:39 PDT. No bounded
baseline capture completed before this freeze.

Saved dump analysis finds `framevr::Input` calling `SetForegroundWindow`;
the re-entered game window procedure waits on a native SRW lock. The updater
also waits for the transport mutex. The candidate now posts a short-lived focus
request for the owning window thread, withholding input injection until a later
frame confirms focus. A native message-queue regression test checks deferred
activation outside a held engine lock, coalescing, expiry and cancellation.
All 25 tests then passed. Logs show Akara opening/closing twice in that session,
continued gameplay and normal Save & Exit; no repeat freeze was reported.

Previous performance DLL SHA256 (backed up, superseded above):
`9b3ce9df5ba307222c5d1d04848ed74ab2aeec83a6f1e4325155268a55f075f7`.
It combines the preserved CPU caches, asynchronous HUD painter and reload guard
with native Windows build compatibility and the focus fix. It is a different
artifact from the archived Linux-built candidate. No native speedup is measured.

Evidence: `build-frame/windows/akara-freeze-20261010-164318/` (logs, baseline DLL,
thread dump, symbolic backtraces and call-site disassembly), deployment record
`build-frame/windows/latest-candidate-deployment.json`, rollback/save snapshot
`build-frame/backups/windows-perf-20261010-164857/`. Recovery force-closed the
frozen process with explicit approval; current saves were backed up and never
restored. DLL-only replacement verified unchanged INI and graphics hashes.

The baseline log also records rejected spatial UI conversion with working flat
fallback before the hang; the typeless-view fix above addresses its reproduced
format rejection. Repeated `HoverUnit` signature-check failures at RVA `0xF1900`
remain unresolved. Keep signature guards enabled and raw dumps outside Git.

## Get the correct repository and build

Private repository: <https://github.com/bearstonem/d2r-pcvr>.
The active branch is **`vr/steam-frame-proton`**, not the upstream/default
`main` documentation. With GitHub access, the next agent can use:

```powershell
git clone --branch vr/steam-frame-proton https://github.com/bearstonem/d2r-pcvr.git
```

Download the assets from release **`v0.2.0-windows-handoff`**:

- **`d2r-pcvr-baseline.zip`**: start here. Archived pre-performance DLL used for
  the latest Linux comparisons, including current gameplay, belt-follow and
  casting-weapon aim changes. SHA256 of `bin/d2rl-vrcam.dll`:
  `16e24f5564a822531682629a5753c0b5ef248ef48dc33f0fbe358e1e30ed15fc`.
- **`d2r-pcvr-candidate.zip`**: experimental CPU/HUD optimizations and the
  settings-reload visibility fix, built from the handoff source. DLL SHA256:
  `bc51fd3700b014b7a614e45fb15016529e98527fa8a7c4c8187986e16d0aa4ce`.
  This final combination has **not** been headset verified. Do not silently
  substitute it for the baseline while investigating streaming.

Each ZIP contains a Windows x64 OpenXR loader, installer/launch scripts,
documentation, licenses and per-file checksums in `release-manifest.json`.
Adjacent `.zip.sha256` assets check the archives. The baseline's binary predates
the handoff source: its manifest explicitly records archived provenance rather
than claiming it was compiled from that commit. GitHub's automatic source ZIP
contains no compiled plugin. The older `v0.1.0-pcvr-preview` predates the current
gameplay work and should not be used for this handoff.

## Install from nothing

1. Install Steam, SteamVR, the purchased **Steam** D2R, and Python 3.10+ with
   its Windows `py` launcher. Python scripts use the standard library; no
   manual pip packages, Blender or proprietary model tools are required.
2. Launch D2R normally through Steam **before adding the mod or launch options**.
   Complete account/license setup online using the stock game, reach the menu,
   create/select an **offline** character, then fully exit. The owner's Linux
   test character was Nekro; saves are not included in GitHub or the packages.
   If transferring personal saves separately, back up both locations first;
   do not overwrite newer progress or delete the equipment-bearing corpse.
3. Set up Steam Frame's Windows PC streaming connection and start SteamVR.
   Select **SteamVR as the active OpenXR runtime** in its OpenXR settings.
   Confirm the dashboard/another PCVR title streams without stalls first, and
   record any picture breakdown before D2R is running. Do not assume Windows
   automatically cures the Linux stream failures.
4. Extract **the baseline ZIP** to a stable directory, for example
   `C:\Mods\d2r-pcvr-preview`. Keep this directory: the Steam launch wrapper
   runs from it. Use Steam → D2R → Properties → Installed Files → Browse to
   determine the actual game path; do not assume drive C.
5. In PowerShell, with D2R closed:

   ```powershell
   Set-Location 'C:\Mods\d2r-pcvr-preview'
   py -3 --version
   Get-FileHash .\bin\d2rl-vrcam.dll -Algorithm SHA256
   $D2RGame = 'C:\Program Files (x86)\Steam\steamapps\common\Diablo II Resurrected'
   py -3 steamvr/install.py --game "$D2RGame" --plugin bin/d2rl-vrcam.dll --openxr-loader bin/openxr_loader.dll --deps deps --download-deps
   ```

   Confirm the hash matches the baseline above. The installer checks the
   supported game version and pinned dependency hashes. If the Steam game has
   updated, retain that version check and adapt/test signatures; do not bypass
   it or download an alternate game executable. Downloads come from the
   official D2RLoader/ReShade sites. See [THIRD_PARTY.md](THIRD_PARTY.md).
6. Retain the backup path printed by the installer. It backs up replaced files,
   settings and saves. It preserves stock `D2R.exe` and adds `D2RLoader.exe`,
   the plugin and mod-local assets. It extracts icons and all 15 potion models
   from **this machine's own game archives**. No extracted Blizzard assets are
   shipped. On Windows the installer uses the system shader compiler; do not
   copy Proton/Wine DLLs or Linux environment overrides into this installation.
7. Set D2R's Steam **Launch Options** to the actual extracted directory:

   ```text
   py -3 "C:\Mods\d2r-pcvr-preview\steamvr\launch_windows.py" %command%
   ```

   Keep exactly one `%command%`. Normal Steam **Play** still starts stock D2R
   for legitimate license refresh. Explicit VR launch, with SteamVR/Frame ready:

   ```powershell
   Start-Process 'steam://run/2536520//--frame-vr/'
   ```

   The wrapper changes only the executable in Steam's supplied command, keeps
   Steam's environment/arguments and starts D2RLoader in the game directory.
   Do not manually start an unrelated copy of D2RLoader or replace `D2R.exe`.
8. Choose the offline character. Keep the desktop D2R window focused for menu
   laser clicks/dragging. Test looking, walking, inventory and vendor UI before
   changing graphics or enabling optional features.

If an installed folder or saves location is redirected, pass `--game` and
`--saves` explicitly. Default Windows saves are
`%USERPROFILE%\Saved Games\Diablo II Resurrected`. Battle.net installations are
not adapted to these Steam wrappers. Install permissions, wrapper execution and
native Windows behavior still require validation on the actual Windows machine.

## First-session settings and controls

Fresh installs deliberately set **motion controls on, fixed foveation on,
eye-tracked foveation off**, symmetric projection on and conservative
`[render] defer_gpu_wait=0`. Start there. Eye tracking was verified on Linux
Frame with sharing permission, but enable it on Windows only after confirming
gaze availability and baseline stability. Quest 3 has no eye-tracking hardware.

The installer chooses **80% resolution scale, medium shadows/AO/atmospherics,
FXAA, DLSS off, VSync off, dynamic resolution off and no frame cap**.
These are conservative fresh-install defaults, not the last Linux comparison:
Linux used **100%, shadows/AO 4, DLSS off/FXAA, 3840×2160 eye images, 90 Hz,
room rings 2/model radius 300 and both fixed/gaze foveation on**. Record Windows
settings/resolution/refresh; do not compare FPS as if those scenes/settings match.
Do not automatically copy the entire old `Settings.json` or INI into Windows.

| Control | Action |
| --- | --- |
| Left stick / right stick sideways | Walk / snap turn |
| Left Grip + left stick click | Reset eye height; stick click alone is map |
| Left X (some controllers label it left A) | Native tabbed inventory/character/skills panel |
| Right-hand laser + Right Trigger | Menu click/drag; keep desktop game focused |
| Right B in native panels | Back / Escape |
| Left Grip + Left Y | VR settings panel and control layout |
| Left Trigger + face button | Quick-cast; add Left Grip for alternate LT slots |
| Right Grip at a belt bottle | Grab; bring opening to mouth and tip briefly to drink |
| F11 | Full view recenter |

See [GAMEPLAY.md](steamvr/GAMEPLAY.md) and
[INTERACTIONS.md](steamvr/INTERACTIONS.md) for the wheel/editor, holsters,
contact melee, bow, potions and UI input ownership. Physical combat is generic
across equipped weapons; do not make character-specific hacks. No spell
gestures are requested: equipped spell + aimed trigger is the intended casting.
Spell aiming with Teeth follows the weapon shaft in the native game plane;
full hand-origin/elevated 3D projectile flight is still unfinished.

## Why we are moving to Windows; exact stopped state

- Source parent before this handoff: `7a3fca1`, native panels/quick casts/potions.
  The new source also contains body-following belt, casting-weapon aim, CPU hook
  optimizations, optional all-thread metrics, asynchronous HUD atlas painting
  and a guard against visibility rebuilds on unrelated settings reloads.
- Linux player experienced freezes with severe picture degradation. Restoring
  the older DLL, wait=0/radius=300 **did not fix it**. Rebooting the PC and
  starting fresh SteamVR/Frame **also did not fix it**. User confirmed both.
- On the fresh boot, SteamVR Link logged **119 stream reset requests before
  D2R launched**. This establishes a fault outside the game/mod. The preceding
  night had 147 requests in ~4 minutes and reported send→receive delays up to
  734 ms, with much shorter game submission/encoder stages. This is evidence
  of delivery/receiver problems, not proof of a particular radio/driver cause.
- Separate native simulation ticks reached 173–753 ms. Some other stalls occur
  during the VR begin stage, which includes input and other work as well as XR
  calls. Do not attribute every stall to networking, graphics or shader builds.
- A **50 Mbps manual stream test was started but not evaluated**: the owner
  chose Windows migration before feedback. We restored automatic bandwidth
  **on** and target **200 Mbps**, with readback confirmed. No result can be
  claimed for the 50 Mbps test. Game graphics were unchanged.
- At the Linux handoff there was no native Windows run. The current Windows
  result is recorded above; controlled performance and broader UI checks remain.

Portable quantitative evidence is in
[steamvr/evidence/2026-10-10-handoff.json](steamvr/evidence/2026-10-10-handoff.json);
interpretation and older controlled comparisons are in
[PERFORMANCE.md](steamvr/PERFORMANCE.md). Raw logs, game memory snapshots and
personal saves remain on Linux and are not needed to begin this Windows setup.

## Next agent's diagnostic workflow

1. Confirm exact DLL hash, game build, runtime, GPU, eye dimensions, refresh and
   graphics settings in the new environment. Verify offline startup and native
   Windows UI/controls before performance conclusions.
2. Establish stream stability outside D2R first. Preserve SteamVR's
   `logs/driver_vrlink.txt`, `vrcompositor.txt` and `vrlink_aftm_*.csv` under the
   actual Windows Steam installation when present. Do not assume Linux paths,
   Wi-Fi driver findings or Wine costs apply to Windows.
3. Preserve `GAME/d2rloader/logs/d2r-vr-frame.log`, startup
   `d2r-vr-vrcam.log`, `d2rloader.log` and `GAME/ReShade.log`. Check OpenXR/D3D12
   startup errors before changing camera/shader math. The baseline's centered
   projection, current-eye labels, smoke/portal facing and body alignment were
   already visually verified on Linux; avoid undoing them as generic guesses.
4. For a bounded game profile, set `[debug] profile_gpu=0` and
   `profile_capture` to a new positive integer; wait for `Frame profile,N,360,`
   in the log, then return capture to 0. Run
   `py -3 steamvr/profile_report.py PATH_TO_LOG --sequence N`.
   The archived baseline still forces a 500 ms visibility rebuild on INI reload,
   so exclude that settling interval or use an unchanged capture procedure for
   A/B. The staged source fixes it. `profile_hooks` exists only in the candidate;
   keep it off for ordinary timings. GPU profiling forces the completion fence.
5. Analyze saved Link CSVs with `steamvr/stream_report.py`. PowerShell does not
   expand glob arguments for native programs like Bash; pass resolved paths:

   ```powershell
   $LinkLogs = 'C:\Program Files (x86)\Steam\logs'
   $LinkCsv = Get-ChildItem "$LinkLogs\vrlink_aftm_*.csv" | ForEach-Object { $_.FullName }
   py -3 steamvr/stream_report.py @LinkCsv --driver-log "$LinkLogs\driver_vrlink.txt" --json stream-report.json
   ```

   These fault-triggered, overlapping histories are not an unbiased FPS/loss
   survey. The script merges duplicates, handles 32-bit clock rollover and
   leaves missing timings unknown. No bitrate/resolution is present in them.
6. Only after a usable baseline, test the candidate DLL with **Save & Exit**
   first. Back up `GAME/d2rloader/plugins/d2rl-vrcam.dll`, copy only the candidate
   DLL from its separately extracted ZIP, verify hash, and keep INI/graphics
   unchanged. Do not rerun the full installer merely to swap a DLL: it resets
   configuration. Roll back the same way if visuals/controls worsen.
7. Record user feedback and scene-matched timings separately. Do not publish a
   performance win based on averages while freezes or streaming resets remain.
   Shared eye work, transport ownership/threading, and refresh changes remain
   future experiments. Spell/belt feature checks also remain on TASKS.md.

The original Linux→Windows cross-build is retained. A native Windows
LLVM-MinGW build is now available and verified with automated checks; see
[native build instructions](steamvr/README.md#native-windows-build).
Start headset tests with the packaged baseline. The native source build is an
additional candidate, not a byte-identical reproduction of that archived DLL.
Default MSVC has not been verified. Do not use the upstream BodyWalk installer
for this port.

## Recovery

For the license warning, fully exit and use normal Steam Play to refresh the
license **unmodded**, then exit and launch with `--frame-vr`. The repeated
Linux `services.exe` failure was Wine-specific; diagnose Windows separately.
If stock Play loads the mod, check for stale third-party proxy DLLs/launch
options and the exact wrapper command; preserve originals in a backup.

To undo the installation while preserving current character progress:

```powershell
py -3 steamvr/install.py --game "$D2RGame" --restore 'PATH_PRINTED_BY_INSTALLER'
```

Clear Steam's wrapper launch options when removing/moving the installation.
Further source history and the complete backlog are in [HANDOFF.md](HANDOFF.md)
and [TASKS.md](steamvr/TASKS.md). Windows setup/streaming stability is now the
priority; the earlier DevilutionX project remains parked.
