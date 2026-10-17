# Kurast roof-retention experiment

Status: local developer preview. Headset results are pending; this is not a
production fix and is not called by the installer or launcher.

The owner reports that Act III / Kurast buildings lose their roofs when entered,
leaving the sky visible from inside. The native Kurast HD presets attach
`FadeTargetComponent` to roof models. In Steam D2R 3.3.93787, 33 Kurast presets
contain 728 such roof entities. This provides a concrete cutaway mechanism to
test separately from fog and wall opacity.

`vr/vrcam.cpp`'s `solidwalls` code changes `transparentWallAlpha`; it does not
remove these roof fade links. Added fog cannot restore absent geometry. The
sampled native stone roof center and slum hut roof corner meshes both contain
downward-facing triangles, but that does not prove every roof has a complete,
correctly lit underside.

## What the experiment changes

`steamvr/prepare_roof_preview.py` reads the owner's local game archives and
generates loose preset overrides exclusively in the existing isolated
`mods/pcvr-dev/pcvr-dev.mpq` environment. It removes `FadeTargetComponent` only
from entities whose entire model selection consists of recognized Kurast roof
models. It keeps geometry, transforms, physics, walls, other fade targets and
dependencies intact. Entities with mixed or unknown model variations are left
alone. This deliberately tests only the native roof cutaway links; it does not
add replacement ceilings or change the outdoor sky.

No game assets belong in the repository, PR or release attachments. The generated
presets stay on the owner's machine and are identified by a local receipt. The
script refuses existing overrides, changed receipt-owned files, linked paths and
development metadata that points at normal saves. Removal only deletes matching
receipt-owned files. It neither reads nor edits character saves.

This preview also retains roofs in overhead views **inside pcvr-dev**. A future
production implementation must account for view switching and compatibility with
other mods; do not automatically add this experiment to normal installation.

## Install and test

Use an already configured developer build with exact pcvr-dev metadata and its
own plugin/settings. Close D2R and D2RLoader first. From the topic checkout:

```powershell
.\deps\python\python.exe steamvr/prepare_roof_preview.py --game "YOUR GAME DIRECTORY"
```

No DLL rebuild is needed because this experiment changes local preset data only.
Restart using the isolated developer shortcut (`-mod pcvr-dev`). Use an offline
test character and select **DEV > Scenic Locations > Kurast Bazaar**. Walk into
several buildings, look up, and walk back out. Check for retained roofs, missing
undersides, dark patches, doorway visibility and any unusual performance cost.
Lower and Upper Kurast have different roof families and need separate checks.

To restore native preset behavior, close the game and run:

```powershell
.\deps\python\python.exe steamvr/prepare_roof_preview.py --game "YOUR GAME DIRECTORY" --remove
```

## Verification so far

- Ten synthetic Python checks pass: selective removal, mixed variations,
  preservation, preset scope, collision refusal, install/remove lifecycle,
  partial-install cleanup, modified-file refusal and save-path isolation.
- Read-only comparison against the installed Steam archives finds 33 changed
  presets / 728 roof entities. The only parsed-data changes are removing their
  fade components. None of these selected entities has `WallTransparencyComponent`.
- No new native checks are needed for this Python/data-only experiment.
- Headset appearance, native acceptance of these overrides, runtime performance
  and complete underside coverage remain unverified.

Next: use the headset result to decide whether removing native roof fade links
is sufficient, or whether some model undersides need separate treatment. Keep
that distinction explicit when reporting this experiment.
