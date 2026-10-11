# D2R PCVR v0.3.0 — Windows and Quest preview

Native Windows OpenXR preview for **Steam D2R 3.3.93787**, using **local offline
characters only**. This release includes the merged Windows/performance and
gameplay work, Quest launch routing, the VDXR color correction, and Journal
wheel cleanup.

## Download and install

Download **d2r-pcvr-windows.zip**, extract its `d2r-pcvr-preview` folder to a
permanent location, then follow
[INSTALL-WINDOWS.md](https://github.com/bearstonem/d2r-pcvr/blob/v0.3.0-windows-quest-preview/INSTALL-WINDOWS.md).
The same guide is included in the ZIP. Python 3.10+ and the user's installed
Steam game are required. Choose this compiled asset rather than the automatic
GitHub source archives. A `.zip.sha256` asset and per-file manifest are supplied.

The installer downloads pinned D2RLoader/ReShade dependencies, backs up saves
and replaced files, and prepares icons/potions from the user's own game. No
game files, extracted game assets, saves, raw logs or dumps are distributed.
Keep the extracted folder because Steam uses its launch wrapper.

## What changed

- Native Windows SteamVR setup, guarded native-read performance improvements,
  deferred dialogue focus handling, and typed OpenXR views for UI/potions.
- T-pose body calibration on first world load and through
  **Left Grip + Y → Calibrate body**.
- Swap-hand trigger over **either shoulder** switches weapon sets I/II;
  this replaces the old hip anchors.
- Camera clipping/belt placement corrections, wrist vitals across transitions,
  and native-grid/render-coordinate correction for cave free-swing melee.
- Quest 3 through Steam Link/SteamVR, plus an explicit Virtual Desktop/VDXR
  launch route that selects the runtime only for that process.
- VDXR sRGB swapchain selection fixes washed-out SDR colors, including when
  the runtime also accepts linear UNORM.
- **Journal removed from the wheel**, including its icon and click target.
  Open it through the tabbed game menu with Frame Left A / Quest Left X.

## Run after installation and Steam launch-option setup

Connect the headset first, then use the included shortcut or PowerShell command:

**Steam Frame or Quest with Steam Link/SteamVR** — `Launch-VR-SteamVR.cmd`:

```powershell
Start-Process 'steam://run/2536520//--frame-vr/'
```

**Quest with Virtual Desktop/VDXR** — `Launch-VR-VDXR.cmd`, with SteamVR closed:

```powershell
Start-Process 'steam://run/2536520//--frame-vr%20--frame-vdxr/'
```

Accept Steam's extra-argument prompt if shown and select an **offline character**.
The wrapper consumes the custom flags. Normal Steam Play launches stock D2R.
The mod does not support online play; fully exit VR and use stock Play for
multiplayer. Windows needs neither Proton, BodyWalk nor FlatVR.

## Validation and limits

All **34 native tests**, **24 Python checks** and the generated wheel preview
check pass. A GPU readback regression reproduces the old VDXR gamma error and
passes with the correction. The owner verified Steam Frame/SteamVR and Quest
3/Steam Link on the preceding build, then confirmed normal colors in the VDXR
correction build. VDXR logs tracked outdoor gameplay, 45 accepted native melee
reports, zero position mismatches and normal exit.

The final Journal wheel cleanup has automated/preview validation and has not
had another physical headset session. Exact DLL hashes and coverage are in
the included evidence and
[QUEST_TESTING.md](https://github.com/bearstonem/d2r-pcvr/blob/v0.3.0-windows-quest-preview/steamvr/QUEST_TESTING.md).
Physical **Quest 2**, Virtual Desktop with SteamVR, and Linux/WiVRn remain
unverified. This is not exhaustive class/weapon/feature coverage. Windows
performance was reported as excellent; no controlled baseline gain is claimed.
