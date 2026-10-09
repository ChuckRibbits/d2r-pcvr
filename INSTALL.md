# Install the D2R PCVR preview

Download and extract **d2r-pcvr-preview.zip** from this private repository's
[Releases](https://github.com/bearstonem/d2r-pcvr/releases). Keep the extracted
folder: Steam's launch wrapper runs from it. GitHub's automatic source ZIP does
not contain the compiled plugin. Your GitHub account needs repository access.

This is an **offline single-player** preview for the **Steam edition of D2R
3.3.93787**. Use your own purchased game and offline character. Refresh the
product license by launching the original game through Steam while connected
to the internet, then exit before installing. No game files or saves are shipped.

**Verified:** Linux + Proton Experimental + SteamVR + Steam Frame, first-person
gameplay. The latest source fixes creature name/health stereo depth and the
reported square lighting patches. **Untested:** Quest 3, WiVRn, native Windows
and Virtual Desktop. Belt/health/mana presentation is unfinished. Logging hitches
improved in headset tests, but remaining
stalls and graphics issues are open. This is a test build, not a finished mod.

## Linux + Quest 3 + WiVRn (friend's likely setup)

1. Install Steam, D2R, **Python 3.10+**, and Proton Experimental. Launch stock
   D2R once, finish account/license setup, create an offline character, and exit.
2. Install [WiVRn server and its Quest client](https://github.com/WiVRn/WiVRn#usage).
   Connect the Quest before launching the game. First verify another OpenXR game
   streams successfully. Follow the dashboard's Steam launch/environment guidance.
   WiVRn uses its own OpenXR runtime; Steam is the launcher and **SteamVR is not
   required for native OpenXR apps on this path**. Its Flatpak includes xrizer /
   OpenComposite for OpenVR compatibility.
3. From the extracted preview folder, install (replace the game path):

   ```sh
   python3 steamvr/install.py \
     --game "$HOME/.local/share/Steam/steamapps/common/Diablo II Resurrected" \
     --plugin bin/d2rl-vrcam.dll --deps deps --download-deps
   ```

   The installer downloads checksum-pinned D2RLoader 1.3.1-beta and ReShade
   6.8.0 with full add-on support from their official sites. On Linux it also
   downloads Microsoft's Windows SDK package (about 160 MB) and extracts its
   shader compiler, installed privately under the mod. This resolves ReShade
   compilation staying busy with Wine's compiler; the older Winetricks compiler
   lacks the shader target ReShade needs. It backs up saves,
   replaced files and video settings, and prints the backup location. It keeps
   the original executable and uses no automatically loaded `dxgi.dll`.
4. Run `chmod +x steamvr/launch.sh steamvr/run-proton.sh`. Set D2R's Steam
   Properties → General → Launch Options to the actual extracted path:

   ```text
   "/absolute/path/to/d2r-pcvr-preview/steamvr/launch.sh" %command%
   ```

   If WiVRn supplies a required environment prefix, place it **before** this
   wrapper; retain only one `%command%`. Have your agent use the dashboard's
   current instructions rather than copying a prefix from another machine.
5. With WiVRn connected, run `./steamvr/run-proton.sh` from the extracted folder.
   Choose **Play** with your offline character. Normal Steam Play remains stock;
   the separate command supplies `--frame-vr` to select D2RLoader.

**For your agent:** WiVRn compatibility has not been established. Confirm the
active OpenXR runtime and Proton's WineOpenXR bridge. The Windows game needs
`XR_KHR_D3D12_enable`; the bridge must expose it over WiVRn's Vulkan runtime.
If the log reports a missing extension, stop and resolve the runtime/bridge
configuration. Do not remove the extension check or assume SteamVR settings apply.
Use [HANDOFF.md](HANDOFF.md) for code, diagnostics and current limitations.

For a SteamVR-based Linux route, [ALVR](https://github.com/alvr-org/ALVR) supports
Quest 3: connect through ALVR, start SteamVR and select SteamVR as the active
OpenXR runtime, then use the same mod install/launch steps. That headset path is
also untested by this project. Steam Frame uses Steam's PCVR connection directly.

## Windows + Quest 3 + Virtual Desktop (alternative, untested)

1. Install the Steam edition of D2R, SteamVR, and
   [Python 3.10+](https://www.python.org/downloads/windows/) with the `py` launcher.
   Launch stock D2R through Steam once, complete license/account setup, create
   an offline character, and exit.
2. Install [Virtual Desktop on Quest and its Windows Streamer](https://www.vrdesktop.net/).
   Connect the headset, launch SteamVR and set SteamVR as the active OpenXR
   runtime in SteamVR's OpenXR settings (enable advanced settings if needed).
   Use the SteamVR route for this first test. Verify another PCVR game works.
3. Open PowerShell in the extracted preview folder and run (replace the path):

   ```powershell
   py -3 steamvr/install.py --game "C:\Program Files (x86)\Steam\steamapps\common\Diablo II Resurrected" --plugin bin/d2rl-vrcam.dll --openxr-loader bin/openxr_loader.dll --deps deps --download-deps
   ```

   The supplied Khronos **OpenXR loader** is needed for native Windows; Proton
   uses its own WineOpenXR bridge. It is installed beside D2RLoader.
4. Set D2R's Steam launch options (replace the extracted path):

   ```text
   py -3 "C:\Mods\d2r-pcvr-preview\steamvr\launch_windows.py" %command%
   ```

5. To launch VR from PowerShell, run:

   ```powershell
   Start-Process "steam://run/2536520//--frame-vr/"
   ```

   The wrapper preserves Steam's environment and replaces `D2R.exe` only for
   this explicit VR launch. Ordinary Steam Play stays stock. Native Windows
   injection/driver/runtime behavior still needs a real Windows test.

## Controls and settings

Current source also includes an opt-in [motion gameplay prototype](steamvr/GAMEPLAY.md):
melee swings, bow draws, a hand palette and grab-to-mouth belt potions.
Bow drawing and free-swing melee have been tested on Steam Frame; other
interactions remain experimental. It is disabled by default and absent from the
published preview ZIP. Build from source and read the control changes before
enabling it.

**Keep the D2R window focused on the PC while playing.** Menu laser clicks and
dragging require desktop focus, even if the headset still displays the game.
After using another desktop app, select D2R again before interacting with menus.
Left X can open inventory without desktop focus, but laser clicks cannot.

| Control | Action |
|---|---|
| Left stick | Walk |
| Right stick left/right | Snap turn |
| Left grip + left stick click | Reset eye height |
| Left stick click alone | Map |
| Left X | Open inventory / close open menu panels |
| Right-hand cyan laser + right trigger | Click/drag on menu panels |
| Right B in a menu | Back / Escape |
| Right A in the world | Native interaction / loot target (hand-ray selection unfinished) |
| F11 | Full position, height and facing recenter |
| F4 | First person (native VR keeps this mode; legacy view shortcuts are ignored) |
| F8 | Diagnostic screenshot of both eyes; expect a brief capture pause |

The installer selects 80% scene resolution, medium shadows/AO/atmospherics,
native height fog, VSync off, DLSS off and non-temporal anti-aliasing. Fixed
foveation uses a full-quality center and 2x2 peripheral shading on supported
D3D12 VRS hardware; unsupported devices stay at full quality. This default uses
a fixed center. To compare, set `[render] fixed_foveation=0` in
`GAME/d2rloader/plugins/d2r_vr.ini`. Settings reload live.

Current source also includes an **experimental Steam Frame eye-tracking mode**,
disabled by default and absent from the published preview ZIP. See
[the prototype settings and limitations](steamvr/README.md#eye-tracked-foveation-prototype) below
the fixed-foveation description. It requires actual OpenXR gaze input from the
connected headset. Keep it off for Quest 3; changing streaming software cannot
provide eye-tracking hardware that the headset does not have.

The installer leaves **DLSS off**, the current clarity baseline. Balanced initially
passed a Steam Frame stability check after two stereo-compatibility fixes, but
the owner subsequently found DLSS off much clearer. Older preview binaries lack
those fixes. Performance gains have not been measured in a matched comparison.
The latest development build separates the correction
shader from diagnostics, reducing its isolated compilation from roughly 23 seconds
to 33 ms on the test PC. This startup improvement is not an FPS benchmark.

## Troubleshooting and removal

- **Square light patches that differ between eyes:** the latest development build
  enables `[render] symmetric_projection=1` in `d2r_vr.ini` (reloads live).
  This removed the reported waypoint patches in an initial Steam Frame test,
  with some image softness. Other scenes/headsets are unverified. Set 0 to
  restore the original projection; older preview binaries lack this option.
- **Creature labels at the wrong depth:** the latest build enables
  `[hud] monster_draw_order=1` to paint health/name boxes using the current eye's
  updated position. The owner verified this fix on Steam Frame. Older preview
  binaries lack it; check the release notes when downloading a build.
- **Different library/prefix or redirected saves:** pass `--game` and `--saves`
  explicitly. Linux defaults to the game's Steam library prefix, Windows to
  `%USERPROFILE%\Saved Games\Diablo II Resurrected`. Battle.net installations
  have not been adapted to these Steam launch wrappers.
- **30-day license warning:** exit; use normal Steam Play, without `--frame-vr`,
  to refresh the license online. Then exit and try offline VR. Keep the same
  game installation and Proton prefix. Do not replace the original executable.
- **No VR / flat view:** check the headset is connected and the correct OpenXR
  runtime is active. Read `GAME/d2rloader/logs/d2r-vr-frame.log`, the startup
  `d2r-vr-vrcam.log`, and `GAME/ReShade.log`. Linux Proton logs are in the
  extracted folder's `build-frame/`. Share log excerpts with your agent.
- **Menus:** click/focus the game window on the PC if pointer input is ignored.
- **ReShade stays “Compiling”:** reinstall with `--download-deps` using the
  current installer. Linux needs the pinned SDK compiler in
  `d2rloader/runtime/d3dcompiler_47.dll`; stock launch and prefix DLLs stay intact.
- **Remove:** save and exit D2R; run the command below with the backup path
  printed during installation. Use `py -3` instead of `python3` on Windows.
  This restores replaced files and video settings, preserving current character
  progress. Remove Steam's launch options afterward.

  ```sh
  python3 steamvr/install.py --game "/your/game/folder" --restore "build-frame/backups/YOUR-SNAPSHOT"
  ```

Own licenses and upstream attribution are retained. Dependencies are downloaded
from [D2RLoader](https://d2rloader.net/) and [ReShade](https://reshade.me/), rather
than bundled with the preview. See `THIRD_PARTY.md` and `LICENSE`.
