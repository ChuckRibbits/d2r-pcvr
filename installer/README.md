# Native Windows setup EXE

`D2R_PCVR_Setup.iss` builds the graphical installer for the native OpenXR mod.
The older `D2R_VR_Setup.iss` and `build_installer.ps1` belong to the original
upstream implementation and are not used by this build.

The wizard and portable CMD setup share `steamvr/setup_windows.py` for the
installation and Steam configuration. `steamvr/setup_native.py` adds unattended
selection validation, ZIP backup migration, per-copy receipts and managed
removal. The EXE includes an isolated CPython runtime; it does not require or
modify system Python. Loader/ReShade downloads retain the existing pinned hashes.

## Build

Use a clean committed topic branch. Build the native plugin as described in
`CONTRIBUTING.md`, then package its output:

```powershell
.\deps\python\python.exe steamvr/package.py --build build-frame/windows-native --openxr-loader build-frame/windows-native/openxr_loader.dll --casclib build-frame/casclib/CascLib.dll --release-tag v0.3.8-windows-preview --output build-frame/releases/v0.3.8-windows-preview/d2r-pcvr-windows.zip
```

Use the actual paths from your native and CascLib builds. When reusing an unchanged
published plugin, pass `--plugin-source-commit` with its original source commit;
do not claim that it was rebuilt from the installer change.

Install [Inno Setup 7.1.0 or newer](https://jrsoftware.org/isdl.php), then run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File installer/build_native_installer.ps1 -PackagePath build-frame/releases/v0.3.8-windows-preview/d2r-pcvr-windows.zip -Version 0.3.8 -ISCCPath 'C:\Program Files (x86)\Inno Setup 7\ISCC.exe'
```

The wrapper verifies the native ZIP manifest and distributable-file allowlist,
downloads official Python 3.12.10 x64 if needed, verifies its pinned archive hash
and Python Software Foundation signature, then compiles a fresh staging folder.
It never stages a game installation or generated artwork/model cache. Output:

- `build-frame/installers/D2R-PCVR-Setup-0.3.8.exe`
- matching `.exe.sha256` checksum and `.exe.build.json` provenance report

The EXE is unsigned unless a release maintainer separately signs it. Signing
requires an appropriate publisher certificate; do not label an unsigned EXE as
signed. Build output and downloaded dependencies stay out of source control.
Publish only reviewed distributables, checksums and player-facing documents.

## Validate

From the repository root:

```powershell
.\deps\python\python.exe -m unittest discover -s steamvr -p setup_native_test.py
.\deps\python\python.exe -m unittest discover -s steamvr -p setup_windows_test.py
.\deps\python\python.exe -m unittest discover -s steamvr -p uninstall_windows_test.py
.\deps\python\python.exe -m unittest discover -s installer -p '*_test.py'
```

`steamvr/session_test.py` targets Linux `/proc` and is not a Windows check.
The graphical backend tests use temporary fixture folders, never the player's
Steam profile or game. They cover both editions, connection choices, repeated
repair, missing backups, Unicode INI paths, rollback, and multi-copy removal.

Check the payload actually embedded in the compiled EXE without installing:

```powershell
.\build-frame\installers\D2R-PCVR-Setup-0.3.8.exe /PAYLOADCHECK=1 /VERYSILENT /SUPPRESSMSGBOXES /LOG=payload-check.log
```

The setup intentionally aborts after validation, so its exit code is nonzero.
Success is the log line `D2R PCVR PAYLOADCHECK PASSED; no game or Steam files changed.`
This check extracts only temporary files, verifies every manifest hash and
does not register an installation or write game/Steam files.

Before a release, use disposable test profiles/installations for a complete
first install, a ZIP upgrade, repeated repair, both editions and Windows Apps
removal. Confirm cancellation and running Steam/D2R stop before changes, then
verify the resulting shortcut in a headset with an offline character. Report
which checks were automated and which were performed on real hardware.

For unattended QA, `/CONFIG="absolute path to choices.ini"` supplies the same
selections as the UI. Silent installation without a config is rejected. Example:

```ini
[setup]
store=steam
headset=Quest 3
connections=steamvr,vdxr
steam=C:\Program Files (x86)\Steam
game=C:\Games\Diablo II Resurrected
saves=C:\Users\PLAYER\Saved Games\Diablo II Resurrected
steamvr=C:\Program Files (x86)\Steam\steamapps\common\SteamVR
profile=LOCAL_NUMERIC_STEAM_ID
mode=repair
backups=
```

Choices accept UTF-8 (with or without BOM) and UTF-16. `store` is `steam` or
`battlenet`; `mode` is `full`, `repair` or `shortcuts`. Use real detected paths
and the local Steam profile directory ID. `backups` is optional: set it to the
original snapshot folder or its parent when upgrading an older mod without a
complete receipt. Do not share player/account config.

## Recovery and removal

Setup copies only its allowlisted helpers into its permanent per-user folder.
Game snapshots and managed receipts are outside Inno's installed-file list;
repair preserves them and removal leaves backups/logs for the player.
The backend journals game updates and restores changes from a failed attempt.
It preserves unrelated Steam configuration edits and never restores character
saves from an older snapshot. ZIP migration checks all copied backup files.

Windows removal first validates every managed copy, then restores mod files and
removes matching Steam entries in one transaction. It runs this operation at
`usUninstall`, before Inno deletes helpers; failure aborts file removal. A stopped
or incomplete setup retains its helpers and Windows uninstall entry for recovery.
No operation disables Windows Security, terminates Steam/D2R or changes the
system OpenXR runtime.
