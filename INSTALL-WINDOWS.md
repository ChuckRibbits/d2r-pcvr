# Install and play D2R PCVR on Windows

This preview supports **SteamVR and Virtual Desktop (VDXR)** on Windows.

**D2R PCVR is a mod installed on top of the original game. You must purchase
and install your own legitimate Steam copy of Diablo II: Resurrected. The
game is not included, and this mod cannot run without it.**

**The release ZIP includes no Blizzard game assets.** It contains the VR mod
and licensed third-party components. The installer reads the required
item/skill icons and potion models from your own installed game and keeps
them in local caches. Do not redistribute those caches or your game files.

You need the **Steam edition of Diablo II: Resurrected 3.3.93787** and a
**local offline character**. The game runs on your PC and streams to the headset.

## Before you start

1. Purchase and install D2R through Steam. Launch it normally once, finish any account
   prompts, create/select an offline character, then fully exit.
2. Set up your headset and both controllers, including floor/room setup:

   | Headset | Connection |
   | --- | --- |
   | Steam Frame | SteamVR streaming |
   | Quest 3 | [Steam Link](https://help.steampowered.com/en/faqs/view/0E2C-406B-9135-38A4) with SteamVR, or [Virtual Desktop](https://www.vrdesktop.net/) with its Windows Streamer |
   | Quest 2 | The same Quest connection options; compatibility is not yet confirmed |

   For Virtual Desktop, use the **Quest app** and install the **Windows
   Streamer** on your PC. Connect until you can see the PC desktop in the headset.
3. Download **d2r-pcvr-windows.zip** from the
   [release page](https://github.com/bearstonem/d2r-pcvr/releases/tag/v0.3.4-windows-preview),
   under **Assets**.
4. Extract `d2r-pcvr-preview` to a permanent folder, such as
   `C:\Mods\d2r-pcvr-preview`. Keep this folder after installation.

## Install

Double-click **Install-Windows.cmd** in the extracted folder and follow its prompts
on the PC desktop. No separate Python install is needed; setup downloads a small,
verified private runtime from [python.org](https://www.python.org/downloads/release/python-31210/).
An internet connection is needed for the required components.

1. Choose **Steam Frame**, **Quest 3**, **Quest 2** or another SteamVR headset.
2. For Quest, choose **Steam Link / SteamVR**, **Virtual Desktop / VDXR**,
   or **Both**. Steam Frame uses SteamVR.
3. Confirm the detected Steam, game and SteamVR folders. Choose your Steam account
   if more than one is present, and confirm the saved-games folder.
4. For an existing mod installation, choose **Update / repair and keep calibration
   and video settings**. A full install applies recommended settings and resets calibration.
   **Only configure Steam shortcuts** skips installing mod files.
5. Review the choices and continue. When prompted, use **Save & Exit** in D2R and
   **Steam > Exit** on the desktop. Steam must be fully closed while setup adds
   shortcuts. This also disconnects headset streaming temporarily.
6. When setup finishes, open Steam, reconnect your headset and play the matching
   **Diablo II: Resurrected VR** entry in your library. Select an **offline character**.

Setup configures D2R's launch options and creates the appropriate Steam library
shortcut automatically, including **Include in VR Library** for SteamVR. Choosing
**Both** creates two entries. Rerunning setup updates these entries without adding
duplicates. Steam's original D2R **Play** button still launches the stock desktop game.
It does not change the system's OpenXR runtime or Windows security settings.

Backups are kept in **setup-backups** inside the extracted mod folder: game files
and saves under **game**, and Steam configuration under **steam-...**. Keep this
folder and **setup-result.json** after installation. Steam backups contain account
configuration; keep them private and do not share them with the mod ZIP.

## Manual installation

Use this section only if you are installing without the setup wizard.
Install [Python 3.10 or newer](https://www.python.org/downloads/windows/) with
the Windows **py launcher**, then check `py -3 --version`.

Open PowerShell in the extracted **d2r-pcvr-preview** folder. Find your D2R
folder through Steam → D2R Properties → Installed Files → Browse.
Replace the example path below with that folder, then run both lines:

```powershell
$Game = 'C:\Program Files (x86)\Steam\steamapps\common\Diablo II Resurrected'
py -3 .\steamvr\install.py --game "$Game" --plugin .\bin\d2rl-vrcam.dll --openxr-loader .\bin\openxr_loader.dll --deps .\deps --download-deps
```

The installer downloads the required components, prepares icons and potions
from your installed game, and backs up your saves and replaced files.
**Keep the backup path it prints** in case you want to remove the mod later.
The download does not include D2R itself or other players' saves.

The installer applies recommended video settings, including FXAA, DLSS off,
80% resolution scale and medium shadows. Running the full installer again
resets the mod's settings, including body calibration; use the upgrade steps
below if you want to keep your settings.

If your saves are in a custom location, add
`--saves 'D:\Your\Saved Games\Diablo II Resurrected'` to the install command.

## Set Steam launch options

The wizard handles this automatically. For a manual installation, find SteamVR's folder through
Properties → Installed Files → Browse. Set its path below to the
`steamxr_win64.json` file inside that folder, then run these lines in the
same PowerShell window:

```powershell
$SteamVR = 'C:\Program Files (x86)\Steam\steamapps\common\SteamVR\steamxr_win64.json'
$Python = py -3 -c "import sys; print(sys.executable)"
$Wrapper = (Resolve-Path .\steamvr\launch_windows.py).Path
if (!(Test-Path -LiteralPath $SteamVR)) { throw 'Set $SteamVR to your installed SteamVR folder.' }
'"{0}" "{1}" "--frame-openxr-runtime={2}" %command%' -f $Python, $Wrapper, $SteamVR
```

Copy the **single command it prints** into Steam → D2R Properties → General
→ Launch Options. Keep exactly one `%command%`.
This setup lets you use either of the included VR launchers.

**Only using Virtual Desktop, without SteamVR installed?** Generate your
launch options with these lines instead:

```powershell
$Python = py -3 -c "import sys; print(sys.executable)"
$Wrapper = (Resolve-Path .\steamvr\launch_windows.py).Path
'"{0}" "{1}" %command%' -f $Python, $Wrapper
```

## Launch and play

Connect your headset first, then open the extracted mod folder:

| Connection | Double-click |
| --- | --- |
| Steam Frame with SteamVR | **Launch-VR-SteamVR.cmd** |
| Quest with Steam Link and SteamVR | **Launch-VR-SteamVR.cmd** |
| Quest with Virtual Desktop | **Launch-VR-VDXR.cmd**, with SteamVR closed |

Accept Steam's extra launch-arguments prompt if shown, then select your
**offline character**.

Steam's normal **Play** button opens the unmodded desktop game. This mod is
for offline play only; use the unmodded game for multiplayer. Always use
**Save & Exit** and fully close D2R before switching headset connections or
returning to desktop play.

## Add Steam library shortcuts

The setup wizard adds these automatically. To add them manually after
completing **Set Steam launch options** above, you can add separate
Steam library entries for each VR connection. These also let you launch from
the headset without opening the mod folder.

1. In desktop Steam, choose **Games → Add a Non-Steam Game to My Library**,
   click **Browse**, and select `steam.exe` from your Steam installation.
   Its default location is `C:\Program Files (x86)\Steam\steam.exe`; use
   your actual Steam folder, which may differ from your D2R library folder.
2. Click **Add Selected Programs**, then right-click the new library entry
   and open **Properties**.
3. Set **Target** and **Start In** to your Steam paths, keeping the quotes:

   | Field | Example |
   | --- | --- |
   | Target | `"C:\Program Files (x86)\Steam\steam.exe"` |
   | Start In | `"C:\Program Files (x86)\Steam"` |

4. Choose the matching name and paste its **Launch Options**:

   | Library name | Launch Options |
   | --- | --- |
   | Diablo II: Resurrected VR - SteamVR | `-applaunch 2536520 --frame-vr` |
   | Diablo II: Resurrected VR - Virtual Desktop | `-applaunch 2536520 --frame-vr --frame-vdxr` |

   These are the shortcut's launch options. Leave the original D2R game's
   Python/wrapper launch options configured as described above.
5. For the **SteamVR** entry, enable **Include in VR Library**. For the
   **Virtual Desktop** entry, leave it disabled so Steam does not start SteamVR.
6. To add the other connection, repeat these steps with a second entry.
   If Steam will not add `steam.exe` twice, add another program as a
   placeholder, then replace its **Target**, **Start In**, name and launch
   options with the values above before using it.

Connect your headset, then select the matching library entry and press
**Play**. On Steam Frame or Quest Steam Link, use the **SteamVR** entry in
the headset's Steam library. For Virtual Desktop, connect through its Quest
app, close SteamVR, and start the **Virtual Desktop** entry from the streamed
PC desktop. Select an **offline character** in the game.

The original D2R library entry's **Play** button still opens the unmodded game.
If a shortcut remains on **Launching**, fully close D2R and check that its
launch options use the `-applaunch` command above.
Steam's [non-Steam shortcut guide](https://help.steampowered.com/en/faqs/view/4B8B-9697-2338-40EC)
also explains how to add a library entry.

## Calibrate your body

The first time you enter the world, a calibration prompt appears:

1. Stand straight somewhere safe, with both feet on the floor.
2. Stretch your arms out sideways at shoulder height, like a T.
3. Release both triggers, then hold them together while staying still.
4. When **Saved** appears, release the triggers to return to the game.

The world stays live during calibration. **Right B** cancels or skips.
Repeat calibration through **Left Grip + Frame Left B / Quest Left Y → Calibrate body**, especially
after changing headset or player. If the prompt cannot find the floor,
complete your headset's floor/room setup and reopen calibration.

## Controls

| Action | Control |
| --- | --- |
| Walk / snap turn | Left stick / right stick left-right |
| Game menu: inventory, skills, character and Journal | **Frame Left A / Quest Left X** |
| Menu select / drag | Point the right-hand laser and use right trigger |
| Laser context / alternate click | Left Grip + Right Trigger |
| Native menu navigation / back | Sticks and face buttons / Right B |
| Inventory: drop selected item | Quest Left X / Frame Left A; follow the native hold prompt |
| Inventory: sort | Press and hold right stick click |
| VR settings | Left Grip + Frame Left B / Quest Left Y |
| Spell wheel | Hold Left Y, turn the left palm toward you, then select with right laser + trigger |
| Assign wheel skills | Spell wheel → **EDIT SKILLS** → slot → learned skill |
| Basic attack / bow on right trigger | Select **WEAPON** in the wheel center; release the selection trigger first |
| Melee | Swing your equipped melee weapon through an enemy, even with a skill selected |
| Manual attack / selected spell | Right trigger |
| Bow | Bring the right hand to the bowstring, hold right trigger, draw back, then release |
| Swap weapon sets I/II | Reach your right hand over either shoulder and press right trigger; change the swap hand in VR settings |
| Seated weapon swap / stow | Wheel → **EQUIPMENT / BELT** → **Weapon I**, **Weapon II** or **Stow/Draw** |
| Grab a belt potion | Reach to a bottle and hold grip with either hand |
| Drink | Keep grip held, bring the bottle opening to your mouth and tip it; release an unused bottle to return it |
| Quick potion | Wheel → **EQUIPMENT / BELT** → select a potion with right trigger |
| Wrist health / mana | Look at the back of your left wrist, like checking a watch |
| Map / height reset | Left stick click / Left Grip + left stick click |
| Recenter | PC F11 |

Keep the D2R window focused on the PC so menu clicks work.
For the full Quest and Steam Frame mappings, including all eight quick-cast
shortcuts and the game's A/B/X/Y/LB/RB/LT/RT menu prompts, see
**[Controller reference](CONTROLS.md)**. Use a stick or face button for native
navigation; release it and deliberately re-aim the laser to resume pointing
and dragging. Laser dragging works in inventory, stash and merchant panels.
Free swings pause while casting, using menus or handling a potion. Selecting
a wheel skill changes the right-trigger action; its displayed quick-cast
binding also remains available.

## If something goes wrong

- **Game opens flat:** use the matching **Launch-VR-*.cmd** file, check the
  Steam launch options and connect your headset before launching.
- **Virtual Desktop cannot launch VR:** install its Windows Streamer in the
  default location, connect the headset and close SteamVR before trying again.
- **An install file is quarantined:** check Windows Security's Protection
  History for the affected file. Installation cannot finish while a required
  component is quarantined.
- **Setup cannot update Steam:** choose **Steam > Exit**, then retry. Closing
  Steam's window leaves it running. Setup does not terminate games or Steam for you.
- **Setup reports an unsupported Steam configuration:** use the manual installation
  and shortcut directions above. The original Steam configuration is kept.
- **D2R asks for a license/account check:** fully exit, launch the game with
  Steam's normal Play button while connected to the internet, finish the prompt
  and exit before launching VR again.
- **Body height or reach feels wrong:** repeat body calibration while standing
  straight with your arms out, and check the headset's floor setup.

## Upgrade or remove

**Upgrade while keeping your settings:** fully close D2R, download and extract
the new release to a permanent folder, run **Install-Windows.cmd** and choose
**Update / repair and keep calibration and video settings**. Setup refreshes
the mod and its launch paths while keeping your calibration and video settings.

For a manual DLL-only upgrade, keep a copy of the existing
`d2rloader\plugins\d2rl-vrcam.dll` and `d2r_vr.ini` from your game folder.
Replace only that DLL with the new release's `bin\d2rl-vrcam.dll`.
Keep your existing INI to retain calibration, and repeat the Steam launch-option
step using the new extracted folder. Run the full installer if the new release
asks you to update its other components.

**Remove the mod:** fully close D2R. Open PowerShell in the extracted folder,
set `$Game` to your D2R folder as in the install step, then replace the backup
path below with the one the installer printed. Wizard backups are under
`setup-backups\game`; manual installer backups are under `build-frame\backups`.
The wizard's private Python can be used without installing Python separately:

```powershell
& .\.runtime\python\python.exe .\steamvr\install.py --game "$Game" --restore 'C:\Mods\d2r-pcvr-preview\setup-backups\game\YOUR-SNAPSHOT'
```

For a manual installation, use `py -3` in place of the private Python path.
Then clear D2R's Steam launch options and remove its VR shortcuts. To restore the
previous Steam configuration instead, fully exit Steam and copy the two VDF files
from the matching `setup-backups\steam-...` folder back to
`Steam\userdata\YOUR-LOCAL-ID\config` (the ID is in `setup-result.json`).
If no `shortcuts.vdf` existed before setup, its backup is absent; remove only the
new D2R VR entries through Steam instead of replacing unrelated shortcuts.
Your current character progress is kept;
the installer restores replaced files and your previous video settings.

See [THIRD_PARTY.md](THIRD_PARTY.md) for licenses and credits.
