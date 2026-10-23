# Knockable level props (Box3D)

Status: first version; not headset-tested yet.

The level's own small props near the player (cups, plates, bowls, vases,
jugs, bottles, books, skulls, baskets, pails and similar) can be knocked about
with swords, other melee weapons, fists, the shield, or a dropped bottle.
A prop stays exactly where the game placed it until something reaches it. It
then falls, bounces and rolls in the same Box3D world as the
[dropped bottles](DROPPED_BOTTLES.md), and freezes again where it comes to
rest. The game draws its own model there, shadow included. This is purely
visual: no native request, item, collision or game state is involved, and
everything returns to its place when the area loads again.
`[interactions] prop_physics=0` turns it off.

## Pieces

- **Catalog** (`steamvr/prepare_props.py`, at install). Every HD level preset
  (`data/hd/env/preset/**.json`) lists its entities with an `id`. The
  installer reads the player's own game files and keeps each entity that is
  only a transform and a model (nothing that lights, emits, animates or
  collides natively) whose model name is one of about 70 kinds. It writes the
  id, the preset position and the kind to `d2r_vr_props/props.bin` beside the
  plugin. That is about 20,000 props from 2,100 presets in roughly 14 seconds
  on Steam 3.3.93787. The catalog is cached by game build and never shipped.
  Candles, lanterns and lamps are left out because their flames are separate
  entities.
- **Scan** (`vr/prop_scan.cpp`, a worker thread, twice a second). The HD
  renderer keeps its world in an entt registry. Pool numbers are runtime type
  numbers, so the three storages that matter are recognised by their contents:
  - the 0xd0-byte transforms: local matrix, world matrix, the previous frame's
    world matrix, and the parent;
  - the 4-byte preset ids, whose values the catalog knows at those entities'
    local positions;
  - the 0x60-byte bounds, whose world box is the local box under the world
    matrix.

  Entities within 3 m of the player and between 0.5 m below and 2.5 m above
  the floor become props when their id and their local position both match
  the catalog. The prop must be between 1 cm and 80 cm in size, have a rigid
  matrix (no mirroring or shear), and belong to a level preset. At most the
  64 nearest are kept. All reads are guarded copies; nothing is called in the
  game.
- **What a prop rests on.** Tables, shelves and counters mostly have no
  native collision. Among the level entities' bounds around each prop
  (excluding other props), the smallest footprint under its centre whose
  height range holds the prop's base is its support. On top of that entity the
  surface is the entity's top; lower down (a shelf inside a cabinet) it is the
  prop's own base. The surface is a 2 cm static plate over that footprint,
  reaching at most 1.5 m either side of the prop. A prop on the floor rests on
  the bottles' ground. With nothing found, a plate just past the prop's own
  footprint holds it until it is pushed off.
- **Physics** (`steamvr/dropped_bottles.*`). Each prop is a static Box3D body
  in the shape of its model's bounds: an upright cylinder for vessels and
  plates, a box for books, skulls, tools and pillows, a sphere for apples and
  potatoes. Its material sets density, friction, bounce and rolling
  resistance. Before each frame's steps, a weapon capsule swept from its last
  pose to its new one (six samples), passing within 1 cm of a prop, sets that
  prop going. So does a bottle or another prop about to reach it. The prop
  turns dynamic and the hand gets the usual light tick. At most 12 move at
  once. A moving prop is frozen again (static, where it is) when it has moved
  under 3 cm/s for 0.6 s, when Box3D puts it to sleep, after 12 s, or 12 m
  from the player. One that falls through everything is set back on the
  floor. A new floor, game or scale restarts the world only once no prop is
  moving.
- **Drawing.** While a prop moves, the transport sends its interpolated pose
  as a renderer matrix, composed with the prop's own scale and its own units
  per metre. Inside the renderer's transform pass (the existing
  `UpdateTransform` hook, once per stereo pair), each matrix is written over
  the cached world matrix. The old one is copied into the previous-frame slot,
  as the game does, so motion vectors stay right. The world bounds are moved
  too, so culling follows. Before every write the entity must still be at
  that index, with the same parent, the same preset position and the matrix
  last put there. Otherwise it is dropped and the physics forgets it. One
  frame after the last move, the previous-frame slot is set to the final pose.

## Limits

- Supports are bounding boxes. A shelf's back and sides are not there, so a
  prop pushed backwards off a shelf falls through the cabinet. Props under a
  table can fall through its top. Sloped or uneven surfaces are flat.
- The player's body and monsters do not touch props. Weapons only do while
  physical melee is active, as for bottles.
- Props that the game builds from several entities move only the catalogued
  part. Props with native collision (crates and sacks that the game itself
  collides with) are not catalogued.
- A moving prop left behind on a reset (headset focus lost, leaving the
  game) stays where it was until the area loads again.
- Steam 3.3.93787 layouts; another game build needs the registry address to
  be found again, and the scan logs why it finds nothing.

## Verification

`props-test`:
- the catalog parser (matching by id and position, malformed files);
- the plank's real matrices and bounds from the developer probe,
  decomposed, composed back and bounded as the renderer does;
- support selection: floor, table top, a shelf inside a cabinet, a huge
  ledge cut to reach, nothing found, scaled units;
- capsule reach, straight through, grazing, beside, over, inside and turned.

`dropped-bottles-test`, in the real Box3D world:
- a cup near a table's edge stays put until a blade sweeps through it, then
  falls off the edge, lands, rests and is reported once;
- a later scan keeps it, and lost and out-of-reach props go;
- a rolled bottle sets a floor cup going;
- a lost moving prop goes at once;
- another floor waits for moving props;
- invalid props are refused;
- no more than 12 move at once.

`prepare_props_test.py` covers kinds, plain-entity filtering, encoding and the
cache.

Headset check (offline character): stand by a table in the Lut Gholein
tavern. Tap a cup with a sword, swing through a plate stack, block a bowl
with the shield and punch a vase. Knock a cup off the table edge, then walk
away and back. Check that the props and their shadows move, that nothing
smears in DLSS, and that leaving and re-entering the area puts everything
back. The log has `Frame props: catalog of ...` at start, then
`Frame props: scans=...` every ten seconds and `Frame props: knocked a ...`
per knock.
