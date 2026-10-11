# Install and run D2R PCVR on Windows

This preview runs the **Steam edition of Diablo II: Resurrected 3.3.93787**
through native Windows OpenXR. Use a purchased game and a **local offline
character**. BodyWalk, FlatVR and Proton are not needed on Windows.

## Choose a headset connection

| Headset | Connection | VR launch |
| --- | --- | --- |
| Steam Frame | SteamVR streaming | SteamVR command below |
| Quest 3 | Steam Link to SteamVR | SteamVR command below |
| Quest 3 | Virtual Desktop and VDXR | VDXR command below; leave SteamVR closed |
| Quest 2 | Steam Link or Virtual Desktop | Same respective commands; physical Quest 2 testing is pending |

The owner tested Steam Frame and Quest 3 with Steam Link, and confirmed normal
colors after the Quest 3 VDXR correction. Individual feature coverage is in
[steamvr/QUEST_TESTING.md](steamvr/QUEST_TESTING.md). This is a PCVR mod, not a
standalone Quest app.

## Before installing

1. Install D2R through Steam. Launch it normally once, complete account/license
   prompts, create/select an offline character, then fully exit.
2. Install [Python 3.10 or newer](https://www.python.org/downloads/windows/)
   with the Windows `py` launcher. Confirm `py -3 --version` in PowerShell.
3. Set up the headset connection. For Frame, use SteamVR streaming. For Quest,
   use [Steam Link](https://help.steampowered.com/en/faqs/view/0E2C-406B-9135-38A4)
   with SteamVR, or the **Quest version** of
   [Virtual Desktop plus its Windows Streamer](https://www.vrdesktop.net/).
   Confirm the dashboard/PC desktop and both controllers work first. Complete
   the headset's floor/room setup.
4. Download **d2r-pcvr-windows.zip** from
   [v0.3.0-windows-quest-preview](https://github.com/bearstonem/d2r-pcvr/releases/tag/v0.3.0-windows-quest-preview).
   Choose the compiled asset, rather than GitHub's automatic source archive.
   This repository is private, so the downloading GitHub account needs access.
5. Extract it to a permanent folder, for example
   `C:\Mods\d2r-pcvr-preview`. Keep that folder: Steam will run its wrapper.
   The optional `.zip.sha256` asset can be compared with
   `Get-FileHash .\d2r-pcvr-windows.zip -Algorithm SHA256`.

## Install the mod

Open PowerShell **inside the extracted d2r-pcvr-preview folder**. Find D2R's
actual folder through Steam Properties → Installed Files → Browse, then edit
the path in this command:

```powershell
$Game = 'C:\Program Files (x86)\Steam\steamapps\common\Diablo II Resurrected'
py -3 .\steamvr\install.py --game "$Game" --plugin .\bin\d2rl-vrcam.dll --openxr-loader .\bin\openxr_loader.dll --deps .\deps --download-deps
```

The installer checks the game version, downloads checksum-pinned D2RLoader
1.3.1-beta and ReShade 6.8.0, and installs the supplied OpenXR loader. It backs
up saves, replaced files and video settings; **keep the printed backup path**.
Spell/item icons and 3D potion models are generated from your own installed
game. No game archives, models, extracted artwork or saves are bundled.

Fresh installation uses motion controls, fixed foveation, FXAA, DLSS off,
80% resolution scale and medium shadows/AO/atmospherics. Installing again resets
the mod's INI and video defaults; back up custom settings before upgrading.

For a different Saved Games location, add `--saves 'D:\Your\Saved Games\Diablo II Resurrected'`.
Fully exit D2R before installation, restoration or a DLL update.

## Set Steam launch options once

For SteamVR/Steam Link, find SteamVR's installed folder through Steam and locate
`steamxr_win64.json`. In the same PowerShell window, edit its path below and run:

```powershell
$SteamVR = 'C:\Program Files (x86)\Steam\steamapps\common\SteamVR\steamxr_win64.json'
$Python = py -3 -c "import sys; print(sys.executable)"
$Wrapper = (Resolve-Path .\steamvr\launch_windows.py).Path
if (!(Test-Path -LiteralPath $SteamVR)) { throw 'Set $SteamVR to your installed SteamVR manifest.' }
'"{0}" "{1}" "--frame-openxr-runtime={2}" %command%' -f $Python, $Wrapper, $SteamVR
```

Copy the **single printed command** into D2R Properties → General → Launch
Options. Keep exactly one `%command%`. This selects SteamVR only for explicit
VR launches. The VDXR launch below overrides this choice for that process;
neither command changes Windows' global OpenXR registration.

For a **VDXR-only PC without SteamVR installed**, use this shorter command
instead to generate the launch options:

```powershell
$Python = py -3 -c "import sys; print(sys.executable)"
$Wrapper = (Resolve-Path .\steamvr\launch_windows.py).Path
'"{0}" "{1}" %command%' -f $Python, $Wrapper
```

## Run in VR

Connect the headset first. Use one of the commands below, or double-click the
matching `Launch-VR-*.cmd` file in the extracted folder. Accept Steam's extra
launch-arguments prompt if shown, then select your **offline character**.

**Steam Frame or Quest with Steam Link/SteamVR:**

```powershell
Start-Process 'steam://run/2536520//--frame-vr/'
```

**Quest with Virtual Desktop/VDXR:** connect Virtual Desktop until the PC
desktop is visible, leave SteamVR closed, then run:

```powershell
Start-Process 'steam://run/2536520//--frame-vr%20--frame-vdxr/'
```

The wrapper consumes `--frame-vr` and `--frame-vdxr`; they are not original
game flags and are not forwarded to D2R. Ordinary **Steam Play stays stock**.
The mod supports offline play only. For multiplayer, fully exit the VR session
and launch stock D2R with normal Steam Play and no VR flags.

Always use Save & Exit and fully close D2R before changing headset/runtime.
Minimizing the desktop game does not close it.

## Calibration and controls

At first offline world load, the body-calibration prompt measures standing
eye height and controller span. Stand straight somewhere safe, stretch both
arms out sideways at shoulder height, release both triggers, then hold both
triggers while still. After **Saved**, release them to return to play. The
world stays live during calibration. Right B skips/cancels.

Repeat through **Left Grip + Y → Calibrate body**, especially after changing
headsets or players. Calibration is shared across headset connections. If the
prompt waits for floor tracking, finish the headset/runtime room setup and
reopen calibration; do not recalibrate while crouching or reaching for a keyboard.

| Action | Control |
| --- | --- |
| Walk / snap turn | Left stick / right stick left-right |
| Tabbed game menu, including Journal | Left primary face button: Frame Left A / Quest Left X |
| Game menu click/drag | Right-hand laser + right trigger; Right B backs out |
| VR settings | Left Grip + Y |
| Spell wheel | Hold Left Y; aim and select with right trigger |
| Free-swing melee | Swing the equipped weapon through an enemy |
| Manual attack / selected spell | Right trigger |
| Swap weapon sets I/II | Reach the swap hand over either shoulder and press its trigger; default right hand, selectable in VR settings |
| Belt potion | Grip a bottle, bring its opening to the mouth and tip it; release unused bottles to return them |
| Wrist health/mana | Look at the back of the left wrist, like checking a watch |
| Map / height reset | Left stick click / Left Grip + left stick click |
| Recenter / diagnostic eye capture | PC F11 / F8 |

Journal is accessed through the tabbed game menu; its duplicate wheel entry is
removed. Shoulder swapping replaces the old hip weapon anchors. Keep the D2R
window focused on the PC for native menu clicks. Further combat and wheel
controls are in [steamvr/GAMEPLAY.md](steamvr/GAMEPLAY.md).

## Troubleshooting, updating and removal

- **Flat/no VR:** check Steam launch options, connect the headset before
  launching and read `GAME\d2rloader\logs\d2r-vr-frame.log`. A normal Steam Play
  intentionally launches stock. VDXR should log `runtime VirtualDesktopXR`;
  SteamVR should log `runtime SteamVR/OpenXR`.
- **Washed-out VDXR colors:** this release fixes the SDR color-format mismatch.
  Its log should show `game format 28, eye format 29`. Check that the installed
  DLL matches `release-manifest.json` rather than an older package.
- **Missing VDXR manifest:** install the Windows Virtual Desktop Streamer in
  its default Program Files location. The wrapper fails with the missing path
  instead of silently selecting a different runtime.
- **Loader quarantined:** the pinned D2RLoader triggered Windows Defender on
  the test PC. The installer does not disable protection or create exclusions;
  installation cannot finish while the dependency remains quarantined.
- **License prompt:** fully exit, launch stock D2R through normal Steam Play
  while connected to the internet, complete the prompt, then exit before VR.
- **Upgrade an existing working install:** with D2R closed, back up the existing
  DLL/INI, replace `GAME\d2rloader\plugins\d2rl-vrcam.dll` with this package's
  `bin\d2rl-vrcam.dll`, and update Steam's wrapper path to this extracted folder.
  Keep the existing INI to retain calibration. For fresh dependencies/assets,
  use the full installer and account for its default-settings reset.
- **Remove:** close D2R, restore the installer snapshot with the command below,
  then remove D2R's Steam launch options. Current character progress is kept;
  replaced mod files and pre-install video settings are restored.

```powershell
py -3 .\steamvr\install.py --game "$Game" --restore 'C:\Mods\d2r-pcvr-preview\build-frame\backups\YOUR-SNAPSHOT'
```

Physical Quest 2, Virtual Desktop with SteamVR, and Linux/WiVRn are not validated
by this release. Windows performance is subjectively good in owner tests; a
controlled baseline comparison has not been completed. This remains a preview.
See [THIRD_PARTY.md](THIRD_PARTY.md) for dependencies and notices.
