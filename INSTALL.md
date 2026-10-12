# Install D2R PCVR

This Windows preview supports **SteamVR and Virtual Desktop (VDXR)**.

**This is a VR mod for a separately purchased game. You must own and install
a legitimate copy of Diablo II: Resurrected.** The original game is not
included. No Blizzard game assets are supplied in the ZIP; the installer reads
the required assets from your own game and keeps its asset caches local.

1. Download **d2r-pcvr-windows.zip** from the
   [release page](https://github.com/bearstonem/d2r-pcvr/releases/tag/v0.3.7-windows-preview).
2. Extract the `d2r-pcvr-preview` folder to a permanent location.
3. Double-click **Install-Windows.cmd** and follow the setup wizard. First
   choose **Steam** or **Battle.net**. Choose your
   headset, SteamVR or Virtual Desktop, then confirm the selected game's folder
   and the Steam account for your shortcuts.
   See **[Install and play on Windows](INSTALL-WINDOWS.md)** for help.

This **v0.3.7 preview** accepts your **Steam copy of D2R 3.3.93787** or
**Battle.net copy of D2R 3.3.93847** (experimental); see
[Battle.net installation](INSTALL-WINDOWS.md#battlenet-preview-packages).
Battle.net has been confirmed with SteamVR; Battle.net with Virtual Desktop / VDXR
has not yet been confirmed.
You also need the Steam desktop client, internet access during setup,
a connected headset and a **local offline character**. Battle.net players
do not need to purchase a second copy through Steam.

After setup, use the matching **SteamVR** or **Virtual Desktop** entry created
in your Steam library. Battle.net entries include **(Battle.net)** in the name.
Keep SteamVR closed for Virtual Desktop / VDXR. Keep the extracted folder;
Steam needs it to launch the mod. The included **Launch-VR-*.cmd** files are
for the Steam edition only.
To add Steam-edition shortcuts manually, follow
**[Add Steam library shortcuts](INSTALL-WINDOWS.md#add-steam-library-shortcuts)**
for separate SteamVR and Virtual Desktop entries.

All Quest and Steam Frame controller mappings, menu buttons, laser interactions
and quick-cast shortcuts are in **[CONTROLS.md](CONTROLS.md)**, included in the ZIP.

## Remove the mod

Run **Uninstall-Windows.cmd**, select **Steam** or **Battle.net**, confirm the
game folder and follow the prompts. Fully exit D2R and Steam when asked.
Current characters and shared stash are kept. The wizard restores the original
backed-up files and removes that edition's VR library entries. It keeps current
video settings by default; you can choose to restore the pre-install settings.
Keep **setup-backups**, including backups in older extracted release folders,
until removal is complete. See [removal and recovery](INSTALL-WINDOWS.md#upgrade-or-remove).

Older packages without **Uninstall-Windows.cmd** can use the manual removal
instructions in the Windows guide.
