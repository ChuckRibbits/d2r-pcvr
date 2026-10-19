# Kurast sewer entrance investigation

Discord report, tracked in [#169](https://github.com/bearstonem/d2r-pcvr/issues/169):
two entrances show missing stairwell surfaces, and entering requires crouching
and looking right. Headset/runtime and reported release version are unknown.

Read-only inspection of the local Steam 3.3.93787 archives confirms that Kurast
Bazaar (80) and Upper Kurast (81) connect to Sewers Level 1 (92) with warp 57.
All four `burbssewer0/1` and `metrosewer0/1` presets reference the same
`act3_kurast_architecture/sewer_entrance02` and
`act3_kurast_stairs/stairs_sewers01` model families. These entities do not have
the roof fade component previously overridden for Kurast buildings.

The two entrance materials (`roof_lip_corner01`, `elevation_walls_a`) and five
stairs materials (`kurast_stairs_A/B`, `elevation_walls_a`,
`pasted__sewer_column_corner`, `pasted__sewer_wall05`) all have native
`IsDoubleSided=0` in the inspected LOD0, 1, 3 and 4 files. The rendering candidate
allows only these exact model families/materials through the existing scoped
two-sided material override, including their normal LOD0-4 variants. Names are
canonicalized before selection. Unrelated stairs, foliage, water, collision,
opacity and native interaction behavior are unchanged. No assets are shipped.
This can restore culled backs; it cannot manufacture missing geometry. In the
owner's outside-entrance test, all seven LOD0 overrides were active, but sky
remained visible through the end of the stairwell. Inspection of the original
LOD0, 1 and 4 meshes found an open floor-end boundary without a closing wall.
The owner exited from inside and inspected the outside; standing re-entry was
not tested.

Warp 57 retains its native 120-by-90 selection rectangle. The existing Interact
guard depends on the native controller target. That makes native targeting a
candidate for the standing/crouching difference, not a confirmed diagnosis.
The read-only DEV trace records native and queried target pointers, type,
class/ID, eye height above the player and view pitch. It never substitutes a
target or bypasses native travel. The diagnostic uses existing
**Equipment > Render debug draws** and starts Off each session.

DEV travel is grouped into Act I-II, Act III and Act IV-V. **Act III > Kurast
Sewers** goes to level 92; take a stair exit outside to inspect its counterpart.
**Upper Kurast** provides the other reported region, alongside the existing
**Kurast Bazaar** shortcut. Test standing selection/entry, both orientations,
inside/outside travel, and whether missing stairwell faces remain. Capture the
active mod's frame log after testing; hook installation alone is insufficient
to claim the materials were overridden or the interaction bug was fixed.

## Local dark-backdrop preview

`steamvr/prepare_sewer_backdrop.py` adds a render-only dark end behind the
stairs in the four native entrance presets. It reuses the game's existing
`black_square01` rectangle, measures the opening from the player's own model
files, and follows each preset's stair transform. Two opposite faces cover
both viewing directions. Existing terrain, entities, physics and warp data
remain intact. This is a local developer preview; normal setup does not run it.
Placement and appearance still need a headset check.

Close the game and its loader, then run from the checkout containing this helper:

```powershell
.\deps\python\python.exe steamvr/prepare_sewer_backdrop.py --game "YOUR GAME DIRECTORY"
```

It requires the existing isolated `pcvr-dev` metadata/save path and refuses
conflicting preset overrides. Generated presets stay inside
`mods/pcvr-dev/pcvr-dev.mpq`; the ownership receipt is
`pcvr-sewer-backdrop-preview.json` there. Do not commit or distribute these
generated files. After restarting **pcvr-dev**, use **DEV > Act III > Kurast
Sewers**, exit outside, inspect the opening from several angles, then try
entering while standing normally. Compare entrances in Bazaar and Upper Kurast.
The visual preview does not establish that the reported entry problem is fixed.

With the game closed, remove only unchanged preview files using:

```powershell
.\deps\python\python.exe steamvr/prepare_sewer_backdrop.py --game "YOUR GAME DIRECTORY" --remove
```

Synthetic checks (no game assets required):

```powershell
.\deps\python\python.exe -m unittest discover -s steamvr -p prepare_sewer_backdrop_test.py
```
