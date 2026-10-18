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
This may restore culled backs; it cannot manufacture missing geometry and has
not yet been visually verified at an entrance.

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
