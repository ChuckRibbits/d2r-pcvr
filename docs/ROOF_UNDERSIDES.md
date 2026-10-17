# Two-sided environment roof materials

Status: candidate; headset verification pending. The owner reported sky showing
through Atma's tavern in Lut Gholein while upper-floor props and beams remained
visible. The existing environmental cutaway hook was installed and had suppressed
a hide request in that session. Local inspection of lower-detail native models
found sparse underside geometry and single-sided roof/floor material metadata.
The highest-detail geometry could not be fully audited with the local decoder;
this diagnosis still needs a rendering comparison.

The native VR plugin now requests the game's own double-sided material variant
for selected building surfaces. It selects both an environment model and its
material, rather than changing culling for the whole scene:

- Dedicated Act I-V environment models whose filename has a `roof` token,
  with materials that also have a `roof` token.
- Atma's tavern `level02` model, including LOD0-4: `ext_floor`,
  `ext_half_circle_roof`, `ext_roof_tile01`, `ext_roof_tile02`, `ext_twig_roof`,
  `ext_roof_trim` and `ext_awning01`. Its upper floor forms the ceiling from below.

The lower tavern model, props, walls, creatures, equipment and unrelated materials
are excluded. This is conservative name-based coverage, not a promise to cover
every building in the game. It cannot fill holes that contain no polygons.

## Native path and isolation

On supported Steam D2R 3.3.93787, material factory `0xFA1700` passes the model
path, file/mesh data, Granny material and two integer arguments to dispatcher
`0xF99C40`. The Standard/VariantMaterial builder asks `GrannyFindMember`
(`0x16153A0`) for `IsDoubleSided`, then consumes the returned integer's low byte
when selecting its rendering variants. The member query returns a definition
pointer and a value pointer; it does not copy the value.

The detour scopes the selected material's extended-data type and object to the
current thread and dispatch call. Only a successful query for that exact object,
field and scalar integer type can return a pointer to a permanent integer 1 in
place of the native zero. All other query results remain unchanged, including
already-double-sided materials. Source material memory is never edited. Nested
non-selected builds clear the scope and restore it on return; other loading
threads have independent scopes. The game's original builder still creates its
normal material/shader variants and retains control of lighting, textures,
opacity, depth and shadow passes. Underside lighting still needs visual testing.

Fingerprints cover both detours, the six-argument dispatcher call, the native
flag query/consumer and the member-result layout. Unsupported code fails closed.
Hooks install before asset loading when available, and retry during startup.
Partial hook installation does not override values. No archives, models or
generated caches are edited or distributed. Unmodded desktop launching does not
load this plugin. Native materials are cached: restart to compare plugin builds;
toggling the camera during a session is not a material-cache reset.

## Validation and test route

`roof-materials-test` exercises the actual detours with synthetic native inputs:
selection and exclusions, malformed/unreadable names, fingerprints, partial
installation, all six arguments and return values, scoped member results,
nested and parallel loads, and unchanged shared source values. The accepted
fingerprints were also compared with the local supported-build capture.
This does not establish live hook activation or visual correctness.

Use the isolated offline developer setup, **DEV > Lut Gholein**, then enter
Atma's tavern. Look at the ceiling from several positions, walk out and back in,
and check that the upper-floor props no longer appear unsupported. Check for
dark/inverted underside lighting, flickering, doorway obstruction and changes
to the exterior. Check a dedicated roof model in Kurast separately.

Runtime logs distinguish `roof undersides: scoped native two-sided material
hooks ready` from a matched model/material with `double-sided overrides=N`.
Only a nonzero override count establishes that the new path ran for that
material. A zero count or no matching-material line needs investigation before
calling this a missing-mesh limitation.
