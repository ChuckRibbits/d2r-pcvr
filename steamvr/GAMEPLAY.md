# Motion gameplay prototype

Right-handed OpenXR controls for offline D2R. The owner verified the live native
bow string/draw and two-hand aiming, then verified revision 4 free-swing melee
on 2026-10-09. A new **physical basic-arrow build is installed for testing**:
immediate native firing on release, a frozen two-hand 3D trajectory, corrected
projectile ownership/speed, timestamp-matched held bow poses, and swept contacts.
The first flight test showed arrows originating above the player. A mesh-only
transform correction is installed; its code/geometry checks pass, and the owner
will test its visual result later. See [../HANDOFF.md](../HANDOFF.md) for the
latest installed build and runtime status.
The published preview ZIP does not include this work.

The **installed development build** adds a radial spell
wheel, native-state belt and wrist gauges, weapon-set holsters and a handheld or
pinned quest page. Detailed headset checks for these interactions remain pending.
The controls below describe this build. [INTERACTIONS.md](INTERACTIONS.md)
separates completed code from the full target design.

The build also adds an **in-headset VR settings panel**.
Hold **Left Grip and press Left Y**, or select **VR SETTINGS** in the wheel.
The shortcut also works with motion combat disabled. Use the right laser and
trigger; **Right B**, the shortcut again, or **Close** dismisses the panel.
The owner verified panel visibility and laser/tab interaction after the
October 9 Proton layer-conversion fix. Apply/cancel and broader input-ownership
checks remain pending.

Spell casting remains **equip, point, pull right trigger**. The candidate extends
hand aiming to spells regardless of the equipped weapon type. Spell gestures are
excluded. Immediate hand-origin 3D spell flight/contact is still pending; native
spell animation, effects, targeting constraints, mana and charges remain active.

Revision 4 matches controller input to the rendered weapon's timestamp and uses
that pose's world placement, fixing missed contacts during fast motion. Its
synthetic timing regression passes, and the owner reported **"works great"** in
Steam Frame on 2026-10-09 after the sword comparison. Other weapon categories and
creature sizes still need checks.

Fresh installs enable motion controls in `GAME/d2rloader/plugins/d2r_vr.ini`:

```ini
[gameplay]
motion_controls=1
```

Settings reload live. Set to `0` to return to the previous button controls.
The installer also enables fixed foveation and leaves eye tracking off. Release triggers/grips
when changing modes. Keep the game focused as before; native game menu laser
clicks still require it. The new mod settings panel handles clicks locally.
Eye-tracked foveation, centered projection and label fixes are retained.

## Controls

Both controllers and the headset must be tracked. Their physical positions and
orientations drive gestures in room space; turning the camera/head alone does
not generate melee velocity. The layout currently assumes a right-handed player.

| Action | Control |
|---|---|
| Inventory / close game panels | Left X, unchanged |
| VR settings and controls reference | Left Grip + Left Y; also **VR SETTINGS** at the bottom of the wheel |
| Spell wheel | Hold Left Y; raise/turn left palm toward you; aim right-hand laser at a wedge and click trigger |
| Select weapon combat | Click **WEAPON** in the wheel center; release selection trigger before attacking |
| Selected skill / spell | Point right hand and press trigger; hold to use the native repeat/channel behavior |
| Melee swing / thrust | Swing the equipped weapon through a creature; no grip, trigger, or preselected target |
| Button attack / crossbow | Weapon combat mode, right trigger; native crossbow firing |
| Nock bow | Bow equipped: bring right hand to string and hold right trigger; no left grip required |
| Draw and shoot | Pull string hand back at least 12 cm (28 cm total separation), then release trigger |
| Cancel active grab/draw | Right B; release inputs before starting again |
| Physical belt potion | Approach a front-belt bottle with either hand; fresh grip grabs it without consuming |
| Drink | Hold grip, bring bottle near mouth and tip nearly horizontal for 0.25 seconds; release grip to discard the preview |
| Quick potion | Left Y → **EQUIPMENT / BELT** → trigger a potion wedge; grip on a potion wedge instead grabs it into the right hand |
| Weapon holsters | Reach either hand to a hip marker, fresh grip then pull out 12 cm; right hip requests set I, left hip set II |
| Seated weapon selection | Utility wheel → **Weapon I** or **Weapon II** |
| Stow/draw | Utility wheel → **Stow weapons** / **Draw weapons**; native equipped stats stay unchanged |
| Wrist health/mana | Turn left palm toward face; brief status cue also appears at low health or low mana in spell mode |
| Quest journal | Utility wheel → **Journal**; initially follows left hand. Squeeze then release left grip to pin; grip near it to pick it back up |
| Journal controls | Right laser + trigger uses native page controls/act tabs; Right B closes it; Left X retains ordinary inventory/menu behavior |
| Quick native weapon swap | Right stick click, unchanged |
| Walk / snap turn | Sticks; paused while wheel or journal is open |
| Height reset / map | Left grip + left stick click / plain left stick click, unchanged |

### In-headset VR settings

The panel opens in front of the headset and stays in room space. It has three
tabs and a **Reset eye height** button:

| Tab | Contents |
|---|---|
| Controls | Left/right controller reference for movement, menus, melee, bow, spells, potions, holsters and height reset |
| VR | Motion combat on/off, seated belt position, holsters, smooth/snap turning, smooth turn speed, belt height and forward reach |
| Video | Foveation Off / Fixed / Eye tracked, runtime capability/gaze status, added sky, fog, dungeon ceilings, terrain and object draw distances |

Click **Apply** to save changed values to `d2rloader/plugins/d2r_vr.ini` and
reload them live. Closing without Apply discards pending edits. The first Apply
creates `d2r_vr.ini.before-vr-menu`; later saves retain that original backup.
Unrelated INI entries are preserved. A conflicting external edit reloads the
current values instead of overwriting them. Reset eye height is immediate and
does not require Apply. The controls tab is a reference, not a binding editor.

Movement and combat inputs are blocked while the panel is open; **the world
continues running**. Release held triggers/grips before resuming gameplay. The
settings laser never sends a native game click. The panel remains accessible
when motion combat is off and from the title/character screen while XR is active.

Eye-tracked shading requires runtime support, permission and valid gaze. Missing
gaze uses full quality; unsupported VRS also renders at full quality. The menu
does not change runtime privacy permissions. DLSS, textures and shadows stay in
the game's **Options → Video**; headset render resolution stays in SteamVR.
There is no in-menu resolution override or control rebinding in this candidate.

First headset check: open/close from the shortcut, then from the wheel; visit all
three tabs and reset height; cancel an edit and confirm it is discarded; Apply
a turn-mode change and confirm it persists on reopening; disable motion combat
and reopen with the shortcut; switch foveation off/fixed/eye tracked and check
clarity and gaze status. Confirm no menu click attacks, drinks or swaps a weapon,
including a held trigger across closing or tracking loss. Also check native
inventory/journal controls after closing. Synthetic previews and automated
tests do not establish headset readability, laser alignment or stereo behavior.

The wheel uses the eight existing native controller assignments: **B, X, Y, RB**
and four **LT +** variants. Assign skills in the native controller UI. Names come
from the game; unreadable assignments show their native button instead. A known
unassigned slot is disabled. A lit slot indicates a present assignment, not a
promise that mana, charges or cooldown permit casting. No spell icons or charge
counts yet. Selecting closes the wheel and latches it until Y is released;
release the selection trigger before casting. **WEAPON** returns from spell mode.

Equipping a different melee weapon or bow selects weapon mode. Javelins and
throwing weapons use free-swing contacts; select the native Throw skill to throw.
Full 3D spell projectiles, reliable terrain-aware destinations and spell-family
adapters remain pending behind the same simple point-and-trigger control.

The belt displays the native next item in each column. A grab captures its item
identity; a refill/reorder cancels an unspent grab. The held preview keeps that
item's color even after automatic refill. It becomes **Used** only after the
native item disappears. A request, grab haptic or mouse click is not proof of
consumption. A refused attempt never automatically retries. Confirm HP/mana and
belt changes in the first test. Success feedback is brief and a grab consumes
at most once. Weapon swaps similarly wait for the observed equipment set.

Holsters represent complete native sets I/II, including shields and quivers;
they do not move arbitrary inventory items. Native set inference still requires
a test with empty/identical sets. Item labels currently show base item codes.
Hands carrying a bottle/book hide the associated weapon; stow hides both.
These are presentation changes only. Combat is suspended while an interaction
owns input, and resumes after release.

Optional live configuration (metres below/forward/from the estimated torso):

```ini
[interactions]
seated=0
waist_below_eyes=0.65
belt_forward=0.26
hip_width=0.28
holsters=1
```

Seated mode raises the waist anchors. Small head glances do not rotate the belt;
anchors freeze during approach/grab. There is no torso tracker or free-form
anchor editor. The journal uses a UI-only copy rather than capturing the world
inside its page. If the UI layer/rectangle/format/copy is unavailable it retains
the ordinary menu surface. Only the quest page is spatial in this candidate;
other menus retain their existing panel. Compositor quads have no scenery/hand
occlusion and the bottle art is a functional placeholder.

### Interaction candidate test order

1. Confirm the installed bow-height correction separately if desired; the new
   candidate includes it too. Keep `free-swing4-20261009-023651` as the verified
   melee rollback. Do not overwrite an active game DLL.
2. Check wheel names against native assignments, select a skill, and keep the
   trigger held: selecting must not cast. Release, then point/press to cast.
3. Use utility set I/II and then grip/pull each hip holster. Confirm the native
   equipment and set label, including an empty set. Check stow/draw.
4. Grab/cancel a bottle; then drink with room for healing/mana. Confirm one item
   consumed, correct next bottle/count after refill, and no accidental attack.
   Try quick-use and a refused/full-health potion without an automatic retry.
5. Check wrist values against the native character screen. Test seated reach if
   needed. Brief warning cues should not become a permanent HUD.
6. Open Journal, turn your head, use a native act tab, pin/regrab, close/reopen.
   The world must remain stereo and clicks must land on the visible page.
   Keep the desktop game focused for injected pointer clicks.
7. Regression: inventory, creature labels, melee, bow, recenter, tracking loss
   and return. No held input should cause a delayed drink, swap, cast or hit.


## What is physical and what is still native

Hand/weapon presentation and input gestures use tracked 6DoF poses. The bow
aim uses the line from the drawing hand through the bow hand. Release freezes
its full position and direction in world space; later head/hand motion cannot
steer an arrow already in flight.

Free-swing melee replaces gesture-to-button attacks. It sweeps a capsule on the
final posed weapon attachment between frames, tests creature body capsules, and
invokes native basic-attack damage for the actual contacted unit on the loader's
authoritative game thread. No controller A press, selected target or attack
animation delay is involved. A contact can hit each creature once per stroke;
settling or a deliberate translation/rotation reversal permits another stroke.
A tracking discontinuity drops that motion and requires 80 ms of fresh coherent
poses; continued swinging can recover without holding the hand still. Strength
is judged over recent motion, so small windups cannot lock out later slashes. The sweep
follows the rotating blade’s arc, including the initial acceleration path. Physical controller
translation/rotation supplies energy, so walking or turning a stationary weapon
into an enemy does not cause damage. Right trigger remains an explicit native
button fallback and suppresses simultaneous physical contact attacks.

Collision volumes are currently **approximations**, not skinned mesh hitboxes:
weapon category determines length/radius on the final attachment; native monster
footprint and legacy artwork height determine its body capsule. Generic class
support is implemented, but specific weapon axes, small creatures, bosses and
sloped ground need headset checks. Main-hand attacks only; independent off-hand
strikes, physical shield blocks and parries remain future work.

Only a confirmed physical contact overrides legacy melee grid reach. Native
line-of-sight, hostile relationship, creature life state, damage, elemental
resistances/immunities, criticals, leech, procs, durability, death and XP still run.
The existing `contact_hits=1` policy removes local-player random accuracy/block/
dodge misses. Contact haptics acknowledge native accepted hit flags, including
hits whose damage an immunity may prevent. Physical stroke timing replaces the
basic attack's animation cadence; attack-speed stat scaling is not yet modeled.

Physical bow release queues one direct native basic attack on the authoritative
game thread. The attack creates its normal damage-bearing missile and handles
ammunition; no simulated A-button press or native animation wind-up is needed.
A client visual is allocated separately on the UI thread, without running a
second attack or spending another arrow. Each release carries a session,
equipment identity, recenter/turn epoch and 150 ms expiry; it cannot be replayed.

Basic arrows follow a straight 3D path from the string hand through the bow hand.
The complete tip segment is tested every native simulation tick, including the
initial shaft, so fast arrows cannot tunnel through body capsules. Native grid
barriers are checked along every crossed cell. The first contact is resolved
through the native missile hit handler, retaining damage, elemental effects,
criticals, leech, procs, immunity and pierce. Piercing cannot damage the same
creature twice. Native lifetime remains in charge; there is no damage multiplier
for drawing farther. Both eyes share the same flying-arrow transform per pair.

This first physical path supports **ordinary basic arrows**, not elemental,
explosive, magic or guided missile variants. A bow that converts its basic attack
to a special missile is currently refused before ammo is spent; use its assigned
native skill through the palette instead. Crossbows and selected skills/spells
keep their native controls, costs, timings and ground-based collision.

Collision is approximate: creature capsules use native footprint/artwork height,
and the floor is the launch floor. Walls extend vertically; arrows cannot fly
over them. The native damage-owner proxy retains its launch grid location while
physical flight and rendering move independently. Effects that query the missile's
native source position, room transitions and uneven terrain need further work;
do not assume exact spatial behavior for every item proc. These limitations and
HD-model alignment require headset checks. Arrows are straight, without gravity.

Recognized bow rigs (initially the hunter's bow) use their existing native mesh:
physical draw amount drives the string, limb bends and nocked arrow continuously.
The string snaps back on release; the later native attack animation cannot play
a second visible draw. Only the bow attached to the current hero is modified,
with rig names/hierarchy, ownership and finite-matrix checks. Bow poses come from
the installed asset's bind geometry, with fixed limb/string lengths. With a valid current attachment
transform, the drawing hand sets the nock
position and the arrow points from it toward the bow hand in full 3D. Unreachable
hand positions are constrained to protect the rig. Invalid attachment data falls
back to a normalized draw (45 cm of extra hand separation). More bow variants
and exact fingertip offsets still need calibration. Projectile
release now invokes the native missile immediately through the physical-arrow
path. Held-bow posing matches the controller sample timestamp to the completed
attachment pose, avoiding a newer hand position being applied to an older rig.

A pull produces light haptic pulses as draw distance changes, plus nock and
release pulses. Runtime acceptance is logged; that does not prove the user felt
it. The native arrow is hidden until nocked. Unknown rigs retain their native
animation plus the crossed-plane arrow and V-string guides.

The palette, neutral bottle and fallback nock/bow guides are lightweight
OpenXR quads, stereo-positioned but not depth-tested against the game world.
The bottle is a flat icon. Actual skill icons/names, belt contents/counts and
health/mana displays need a read-only game-state adapter.
Do not describe this prototype as completed physics combat.

## Cancellation and performance

Menus, focus/tracking loss, weapon changes, recenter and discontinuous samples
cancel unfinished gestures. Held triggers cannot leak from selection/drinking
into a world attack. Tracking must be valid and tracked for both controllers;
after interruptions release the controls before retrying. Frame gaps over
100 ms cancel rather than infer a high-speed gesture from stale poses.

Melee automatically becomes available with a melee weapon. After a tracking
discontinuity it needs 80 ms of fresh coherent samples, without a stationary
pose or grip press/release. A
stroke requires minimum physical speed and net excursion. Its initial acceleration
path is included when it qualifies, and wrist-only backhands rearm independently. Rapid small jitter
cannot accumulate into a strike; a resting blade cannot repeatedly damage a
creature. Menus, skill selection, potion handling, equipment changes, tracking
jumps, snap turns and recentering cancel pending sweeps. Game-thread requests
expire after 150 ms, carry session/equipment identities and are revalidated
before damage. No native unit pointer is passed from the render thread.
Bow shots require a nearby nock, additional deliberate draw, minimum duration,
forward direction relative to the bow hand and actual trigger release. Analog
trigger hysteresis prevents a small squeeze fluctuation from releasing the string.
The grab region is 28 cm around a marker 16 cm behind the bow hand; it accepts
press-before-approach, but a grab begun with separated hands still requires a pull. No arrows
are fired on a tracking-loss release. Potions need a fresh grab and a sustained,
tilted pose near the mouth; consumption requests expire after 200 ms.

The palette atlas is generated/uploaded once at OpenXR initialization (one
1024x1024 texture), using GDI text and simple procedural shapes. Per-frame input
uses fixed state, no heap allocations, texture updates or extra GPU waits.
Additional composition quads are submitted only while their guides are visible.
Measure live cost before making a performance claim.

## Validation and remaining work

Portable gesture tests and the Proton cross-build pass. Tests cover swing
debouncing, head motion, pose discontinuity, bow direction/release/cancellation,
menu click ownership, native slot routing, potion dwell/single-use/cancellation,
invalid poses and palette hit geometry. They do not prove native game behavior
or headset comfort. Run:

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror steamvr/gameplay_test.cpp -o build-frame/gameplay-test-host
build-frame/gameplay-test-host
g++ -std=c++20 -O2 -Wall -Wextra -Werror steamvr/bow_pose_test.cpp -o build-frame/bow-pose-test-host
build-frame/bow-pose-test-host
g++ -std=c++20 -O2 -Wall -Wextra -Werror steamvr/melee_contact_test.cpp -o build-frame/melee-contact-test-host
build-frame/melee-contact-test-host
cmake --build build-frame -j4
```

Headset checks, in order:

1. Open/close inventory repeatedly; palette shows and can be aimed at comfortably.
2. Select an assigned skill; release trigger, aim, press again. Verify costs,
   correct action and repeated/channelled behavior. No attack from selection.
3. Swap sets once per click. Check the held model and gesture mode follow.
4. Melee: any offline character with a melee weapon. Outside town, release all
   buttons and slash, chop or thrust through an enemy. Check health change and
   contact feedback, overhead/beside misses, no damage from resting the blade
   inside a body, one hit per creature per stroke, and no hits from head turns.
   Try a backhand, swaps, menus and tracking recovery. Compare right-trigger
   fallback separately. Repeat with short and two-handed weapons; support across
   categories is an implementation goal, not a claim that every model was tested.
5. Belt: cancel a grab, then drink once; verify native item/count and health/mana.
6. Bow-equipped offline character with arrows: visible marker/arrow and grab/draw
   haptics, no left grip required, no shot on nock, one on release,
   aim follows draw line; short draw, menus and tracking loss do not shoot.
7. Hunter's bow: real string/limbs/arrow must move while holding trigger, remain
   drawn when held still, then reset once on release. Check pull haptics before
   release, menus/cancel/tracking loss, walking, weapon swaps and NPC bows.
8. Check native animations, labels, lighting and frame pacing for regressions.

Next phases: real inventory/skill metadata, waist or wrist belt with counts,
health/mana glance UI, accurate world target/loot highlights, responsive
point-and-trigger projectile casting/ground reticle, refined weapon/body collider
dimensions and two-hand grips, bowstring and arrow meshes, crossbow loading,
physical weapon throwing, haptic success signals,
left-handed and accessible button mappings. Each needs game/headset validation.

## Physical basic-arrow test (2026-10-09)

With `motion_controls=1`, `arrow_speed=2` scales the native creation velocity;
the physical path maps a full-draw ordinary arrow to approximately **40 m/s** at
that setting (20 m/s at 1; supported multiplier range 1–3). Shorter valid draws
reduce speed by up to 30%. Native velocity changes such as Slow Missiles are
read on each tick. This mapping intentionally replaces the older slow native
travel speed; `motion_controls=0` restores legacy controls and flight.

The former ownership check treated the SDK's opaque player handle as a native
unit pointer. It now uses the validated local native player id, including id
zero, and checks owner type/id. That defect could prevent earlier speed and
release corrections from activating. Headset speed improvement is not yet
verified for this revision.

Physical arrows always bypass the stock half-second HD release-bone blend.
`arrow_direct_release` still controls the legacy handoff for other local basic
arrow creation; it cannot reintroduce a delayed blend into a physical shot.
All required native signatures and layout call sites must match before the
physical path activates. Unsupported builds fail closed and log refused/stale
requests rather than synthesizing a second button attack.

The test build logs release/native/visual counts, refusal/stale counts, body
contacts, barriers, expiry, last queue latency and physical speed. It does not
record full hand poses. Host tests cover cancellation/deduplication, high-speed
and vertical contacts, head/side misses, thin walls/corners, speed, bounded
prediction, visual axes, live bow poses and melee regressions. They do not prove
native ammo/HD rendering behavior; verify those in game.

For the next headset pass, use the existing bow character **outside town**:

1. Draw, hold, move both hands, release; confirm immediate flight along the shaft.
2. Fire level, above and beside a creature, then through it. Look for pitch-sensitive
   misses and consistent contacts without preselection or an attack-button hold.
3. Fire beside a wall/corner and at close range; arrows should stop at barriers.
4. Check one arrow spent per release, no shot on B/menu/tracking cancellation,
   and normal behavior on an empty quiver. Fired arrows may finish after a menu opens.
5. Try repeated draws, a weapon swap, leaving/re-entering the game, then a few
   melee swings. Check stereo, lighting and frame pacing for regressions.

## Deterministic player hits (2026-10-09)

The owner verified grip swings and trigger attacks, then requested that valid
melee, arrow and magic contact should connect without dice-roll misses.
With motion controls enabled, `[gameplay] contact_hits=1` now removes random
accuracy and block/dodge/avoid/evade failures for the local player's attacks
against creatures. Set it to `0` to restore the native rolls. This intentionally
changes combat balance; attack-rating/defense values no longer determine these
outgoing hit results. Their ordinary character-sheet display remains unchanged.

For native button/ranged actions, range, line-of-sight, collision/target eligibility,
immunity, damage, criticals, timing and resource costs still run through the game. Arrows and
spells must pass their existing native collision or area checks; a shot that
misses geometrically remains a miss. Incoming enemy attacks, summoned creatures'
attacks and attacks against players retain their normal rules. The modification
uses the identified native player id and current SDK session; SDK handles are
opaque and must not be treated as pointers.

The accuracy hook changes only the specific hit-chance roll and still executes
native success-side behavior such as Prevent Monster Heal. It advances the
original attacker's random stream once normally, keeping the next damage/proc
roll in place. All signatures must match before any override activates.

Free-swing melee now adds a separate swept-capsule contact layer (above), with
physical reach and stroke cadence. The owner verified the sword setup; broader
collider-fit and damage/proc checks remain. Accepted-hit haptics do not prove
positive damage against an immune creature.

Host rule tests and the cross-build pass. Live sword combat on October 9 logged
three eligible melee checks, three accuracy overrides and three block bypasses
without failures. The final free-swing run recorded 58 physical contacts and
58 native accepted hits with no rejections; the owner reported "works great."
Arrow/spell live verification remains open.
