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
2. Install [Python 3.10 or newer](https://www.python.org/downloads/windows/)
   with the Windows **py launcher**. Open PowerShell and run
   `py -3 --version` to check that it is available.
3. Set up your headset and both controllers, including floor/room setup:

   | Headset | Connection |
   | --- | --- |
   | Steam Frame | SteamVR streaming |
   | Quest 3 | [Steam Link](https://help.steampowered.com/en/faqs/view/0E2C-406B-9135-38A4) with SteamVR, or [Virtual Desktop](https://www.vrdesktop.net/) with its Windows Streamer |
   | Quest 2 | The same Quest connection options; compatibility is not yet confirmed |

   For Virtual Desktop, use the **Quest app** and install the **Windows
   Streamer** on your PC. Connect until you can see the PC desktop in the headset.
4. Download **d2r-pcvr-windows.zip** from the
   [release page](https://github.com/bearstonem/d2r-pcvr/releases/tag/v0.3.1-windows-preview),
   under **Assets**. Your GitHub account needs access while this preview is private.
5. Extract `d2r-pcvr-preview` to a permanent folder, such as
   `C:\Mods\d2r-pcvr-preview`. Keep this folder after installation.

## Install

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

Do this once after installation. In Steam, find SteamVR's folder through
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

After completing **Set Steam launch options** above, you can add separate
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
Repeat calibration through **Left Grip + Y → Calibrate body**, especially
after changing headset or player. If the prompt cannot find the floor,
complete your headset's floor/room setup and reopen calibration.

## Controls

| Action | Control |
| --- | --- |
| Walk / snap turn | Left stick / right stick left-right |
| Game menu: inventory, skills, character and Journal | **Frame Left A / Quest Left X** |
| Menu select / drag | Point the right-hand laser and use right trigger |
| Menu scroll / back | Right stick up-down / Right B |
| VR settings | Left Grip + Y |
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
- **D2R asks for a license/account check:** fully exit, launch the game with
  Steam's normal Play button while connected to the internet, finish the prompt
  and exit before launching VR again.
- **Body height or reach feels wrong:** repeat body calibration while standing
  straight with your arms out, and check the headset's floor setup.

## Upgrade or remove

**Upgrade while keeping your settings:** fully close D2R, download and extract
the new release, and keep a copy of the existing
`d2rloader\plugins\d2rl-vrcam.dll` and `d2r_vr.ini` from your game folder.
Replace only that DLL with the new release's `bin\d2rl-vrcam.dll`.
Keep your existing INI to retain calibration, and repeat the Steam launch-option
step using the new extracted folder. Run the full installer if the new release
asks you to update its other components.

**Remove the mod:** fully close D2R. Open PowerShell in the extracted folder,
set `$Game` to your D2R folder as in the install step, then replace the backup
path below with the one the installer printed:

```powershell
py -3 .\steamvr\install.py --game "$Game" --restore 'C:\Mods\d2r-pcvr-preview\build-frame\backups\YOUR-SNAPSHOT'
```

Then clear D2R's Steam launch options. Your current character progress is kept;
the installer restores replaced files and your previous video settings.

See [THIRD_PARTY.md](THIRD_PARTY.md) for licenses and credits.
