# Motion gameplay prototype

Right-handed OpenXR controls for offline D2R. Implemented in source and installed
for the owner's headset tests. The owner verified trigger-only bow draw/release
and native shooting on 2026-10-09, but reported delayed bow animation. The 00:23:59
native-string/limb build was then verified by the owner: live draw and haptics
work. A further two-hand anchor build is installed for testing: the string follows
the right hand and the visible arrow follows the right-to-left hand vector.
The first anchor build regressed because it used the inventory-slot wrist;
a correction now uses the actual left bow attachment and the owner confirmed
it follows both hands. The flying arrow's orientation across its flight path
and its slow travel speed remain open. The flight test was only somewhat aligned
and not noticeably faster; the owner parked it to prioritize generic melee.
Other gesture
outcomes still need individual checks.
The published preview ZIP does not include this work.

Revision 4 matches controller input to the rendered weapon's timestamp and uses
that pose's world placement, fixing missed contacts during fast motion. Its
synthetic timing regression passes, and the owner reported **"works great"** in
Steam Frame on 2026-10-09 after the sword comparison. Other weapon categories and
creature sizes still need checks.

Enable in `GAME/d2rloader/plugins/d2r_vr.ini`:

```ini
[gameplay]
motion_controls=1
```

Settings reload live. Set to `0` to return to the previous button controls.
Installer default is `0` while this is experimental. Release triggers/grips
when changing modes. Keep the game focused as before; menu laser clicks still
require it. Eye-tracked foveation, centered projection and label fixes are retained.

## Controls

Both controllers and the headset must be tracked. Their physical positions and
orientations drive gestures in room space; turning the camera/head alone does
not generate melee velocity. The layout currently assumes a right-handed player.

| Action | Control |
|---|---|
| Inventory / close game panels | Left X, unchanged |
| On-demand hand palette | Hold Left Y; raise and turn the left hand to face the palette toward you |
| Choose a skill or swap weapon sets | Aim right-hand laser at a palette tile; click right trigger |
| Attack mode | Equipping a melee weapon selects it automatically; **Melee / Bow** in the palette returns from spell mode |
| Selected skill / spell | Aim right hand and hold right trigger; native skill determines repeats/channeling, target type, costs and cooldowns |
| Melee swing / thrust | Swing the equipped weapon through a creature; no grip, trigger, laser aim or preselected target. A strong contact resolves a native basic hit |
| Button attack / crossbow | Attack mode, right trigger; crossbow uses native firing, with no reload gesture yet |
| Nock bow | Bow equipped: bring right hand to the string (glowing marker on fallback rigs) and hold right trigger; no left grip required. A pre-squeezed trigger can grab as the hand enters the marker's region |
| Draw and shoot | Pull the string hand back until **Release to shoot** appears, then release right trigger. Minimum 12 cm additional draw and 28 cm total hand separation |
| Cancel bow | Right B; or release trigger before reaching minimum draw. Release trigger after cancellation before grabbing again |
| Take belt potion | Hold Left Y, point right laser at Belt 1–4, press and hold right grip |
| Drink | Keep right grip held, release Left Y, bring bottle to mouth and tip nearly horizontal for 0.25 seconds |
| Cancel potion | Release right grip before drinking; it requests no consumption |
| Quick weapon-set swap | Right stick click, unchanged; palette also has **Swap I / II** |
| Walk / snap turn | Sticks, unchanged; paused while the palette is held open |
| Height reset / map | Left grip + left stick click / plain left stick click, unchanged |

The palette has eight native controller assignments: **B, X, Y, RB** and the
four **LT +** variants. Assign skills through D2R's normal controller skill UI.
The palette changes which assignment the right trigger invokes; it does not
rebind the character or promise a specific skill in an empty slot. The small
tile below the palette shows the current selection. Bind the game's **A** slot
to basic attack for button attacks and the bow test. Free-swing melee invokes
native basic attack directly. This prototype reserves physical Left Y
for the palette; the game's Y-bound skill remains selectable there. Equipping a
bow automatically selects attack mode. Choosing a spell from the palette while
holding the bow still works; select **Melee / Bow** to return to drawing arrows.
Changing to a different melee weapon also selects attack mode. Javelins and
throwing weapons use free-swing contacts as melee weapons. To throw through the
native action, select the assigned Throw skill and use right trigger; physical
throwing gestures are not implemented.

Use the native skill for the intended purpose: directed attacks, buffs and
summons retain their original behavior. Separate physical throwing, two-hand
casting, reliable surface placement and self-cast gestures are future work.

Potions are labelled by **belt column**, not by guessed type. The normal belt
command selects/consumes the native item and obeys its normal gates. Haptics
confirm gesture recognition/request submission, **not successful consumption or
damage**. Verify the actual belt count and health/mana change in the game.
The bottle guide remains until releasing grip; it cannot drink twice per grab.

## What is physical and what is still native

Hand/weapon presentation and input gestures use tracked 6DoF poses. The bow
aim uses the line from the drawing hand through the bow hand, latched across
the short release pulse.

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

Bow/crossbow/spell actions continue to invoke native controls and keep their
native costs and timings. There is no draw-strength damage multiplier, custom
ballistic arrow simulation, or freeform 3D spell collision yet. Existing ranged
aim hooks resolve direction on D2R's ground grid; pitch-sensitive surface
targeting needs further work. Physical hand position does not move the game's
logical projectile spawn away from the hero.

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
release timing remains native, so this does not eliminate its attack wind-up.

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
health/mana glance UI, accurate world target/loot highlights, pitch-sensitive
casting/ground reticle, refined weapon/body collider dimensions and two-hand grips, bowstring
and arrow meshes, crossbow loading, thrown spells/weapons, haptic success signals,
left-handed and accessible button mappings. Each needs game/headset validation.

## Basic-arrow flight test (2026-10-09)

With motion controls enabled, `[gameplay] arrow_speed=2` scales the native
creation velocity of the local player's **basic arrows** on both the client
and offline server. `1` restores native speed; supported range is 1–3. Other
missile types (including elemental arrows), other owners and game time stay
native. Damage, ammo use and attack wind-up are unchanged. Lifetime is still
native, so faster basic arrows may travel farther before expiring.

`arrow_direct_release=1` removes the local basic arrow's native HD release-point
blend. That half-second blend copies the animated release bone into the flying
model, independently of the tracked bow pose. The test instead lets the flying
model follow its native path immediately; `0` restores the original handoff.
This does not yet move the logical arrow spawn to the tracked bow hand.

These changes require matching native signatures. Speed adjustment stays off
until **both** creation hooks and the velocity hook are installed. Logs report
client/server creations, actual adjusted velocities, and direct handoffs.
Host ownership/velocity tests, existing gesture/pose tests and the cross-build
pass. Owner feedback: **somewhat aligned, not that much faster**. Further flight
tuning is parked at the owner's request. The last session logged successful hook
installation but no creation/velocity counters, so actual speed adjustment is
not verified. Keep this separate from the verified held-bow draw implementation.

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
