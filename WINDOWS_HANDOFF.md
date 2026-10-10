# Clean Windows installation and agent handoff — October 10, 2026

Start here for the owner's **fresh Windows installation**, using Steam Frame
over SteamVR PC streaming. This is a native OpenXR mod of the **Steam edition
of Diablo II: Resurrected 3.3.93787**, for **offline characters only**.
Native Windows has not yet been tested. Successful Linux/Proton tests do not
establish Windows injection, rendering or streaming compatibility.

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
- No native Windows run exists yet. No promise of higher FPS, cured streaming,
  correct controller profiles, or compatibility with newer game builds.

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

The documented source build is Linux→Windows cross-compilation with Clang,
MinGW, pinned PluginSDK and OpenXR SDK, not an established MSVC build. Use the
packaged Windows binaries for the fresh setup. If editing on Windows, adapt
and verify a native toolchain or use an appropriate Linux build environment;
do not assume `cmake` with default MSVC reproduces the patched ReShade ABI.
See [technical build instructions](steamvr/README.md#build) and the retained
toolchains/licenses. Do not use the upstream BodyWalk installer for this port.

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
