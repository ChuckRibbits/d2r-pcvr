# Agent handoff — D2R PCVR

Updated 2026-10-09. Read [INSTALL.md](INSTALL.md) for deployment and
[steamvr/TASKS.md](steamvr/TASKS.md) for the complete user-requested backlog.

## Objective and boundaries

First-person Diablo II: Resurrected PCVR. Original user reference was
`emmericp/D2R-3D`; the adapted implementation is MIT upstream
`natpoh/diablo2-vr-port`, commit `10cfb0a2a9080184101794d3ac8c238c35717967`.
This repository retains upstream history/notices and adds a native OpenXR
transport, without BodyWalk/FlatVR as runtime dependencies.

Owner tests on Linux with Proton Experimental, SteamVR and Steam Frame. His
offline character is **Nekro**. Friend has Quest 3, likely Linux with **WiVRn**;
Windows with Virtual Desktop is also possible. Both friend paths are untested.
The instructions deliberately identify that rather than promise compatibility.
Earlier DevilutionX standalone/tabletop and first-person work is parked and is
not part of this repository or deployment.

**Offline single-player only.** Never use modded online characters, bypass license
verification, change the stock executable, remove equipment-bearing corpses or
overwrite progress. Stock Steam Play must stay available for legitimate online
license refresh; the explicit `--frame-vr` launch chooses D2RLoader. Save and exit
before replacing DLLs or restoring files. The installer checks for running D2R.

## Latest user evidence and priorities

**Revision 4 verified in Steam Frame on 2026-10-09.** After the final comparison
request (fast slashes, backhands and swinging while walking), the owner reported
**"works great"** and requested commit/push. This validates the tested sword
setup; it does not establish every weapon/creature combination. No further
captures or headset runs are requested tonight.

Installed 2026-10-09 02:39:27 with the game closed. DLL SHA256:
`6b284346ce23a1118db3122eecb5b2fa59351274df7413d5193e968eb48d88c0`.
The same build is retained at
`build-frame/ready/free-swing4-20261009-023651/d2rl-vrcam.dll` and
`build-frame/d2rl-vrcam.dll`; adjacent `build.json` records validation.
Backup `build-frame/backups/free-swing4-20261009-023927/` contains revision 3
DLL/INI. Build log: `build-frame/build-contact-melee4.log`. Host melee, gameplay,
combat and bow tests and the Windows cross-build pass.

Existing runtime logs confirmed combat/contact hooks ready at 02:39:36. Through
02:47:22 they recorded 58 physical contacts / 58 native accepted hits, 0 rejected,
0 missing-server failures, 1 pose reset and 482 recovery samples. These are
diagnostic counters, not a measured percentage of the owner's swings that hit.
No additional recording was made for this final run.

Code-level defect addressed: `MeleeContactTick` paired the latest XR hand/time
with a cached older `GunBone`, then reconstructed that older bone using newer
camera yaw/eye position. The engine could consume the timestamp before the blade
caught up, or reject contact after the latest hand had stopped. `PoseHistory`
now retains 16 timestamp-keyed input samples; the completed skeleton pose carries
its original shared input stamp. Melee gets that exact sample (max 100 ms), with
current action, equipment and recenter gates still mandatory. World origin/+Y axis
and player placement come directly from final attachment × the hero's same-pose
root transform, not from a later camera frame. Hero generation and snap-turn
world epoch reject obsolete geometry. Bow/debug `GunBone` behavior is unchanged.

Synthetic regression demonstrates the old asynchronous order loses a short fast
slash, while matching pose timestamps retains the contact after the hand stops.
History tests cover unknown/expired stamps, menu/trigger cancellation, equipment,
recenter/tracking changes and zero stamp after wrap. The subsequent owner test
confirmed the improvement. Generic capsule/weapon-fit limitations remain:
cross-weapon/boss/slope checks, attack-speed scaling, offhand contacts and parries
are still open. Bow draw/string posing is verified; flying-arrow orientation and
speed tuning stay parked. Palette, potion and spell ergonomics still need their
own validation. The published preview ZIP predates this motion-combat work;
build from source for these changes.

### Historical motion-combat diagnostics

The entries below preserve earlier investigations and rollback artifacts. Their
pending-test and installation notes describe those older builds; the verified
revision 4 status above is current.

**Revision 3 installed 2026-10-09: slashes were failing despite reliable thrusts.**
The owner reported thrusts work but swings in any direction often miss. A read-only
40-second live capture (1,698 input samples, `build-frame/melee-motion.jsonl`) and
host replay reproduced a **35-second lockout**: real slashes reached 53 rad/s,
tripping the old 35 rad/s guard; the required 80 ms of almost stationary motion
never occurred during continued fighting. The separate 18 m/s derived blade-speed
ceiling also rejected the strongest portions of legitimate swings. Only 119
powered samples / 4 distinct target-stroke contacts survived the old replay.
The first test session recorded 38 contacts / 38 native acceptances / 0 rejections,
so the damage dispatch was not the primary cause of this failure.

`steamvr/melee_contact.h` now recovers after 80 ms of coherent fresh samples,
without requiring stillness. It drops the discontinuity itself; preserves menu,
session, equipment, snap-turn, stale-input and teleport cancellation; uses
rate-aware 10 m/s hand / 60 rad/s rotation guards; permits blade travel justified
by physical hand motion; removes the derived upper-energy cutoff. Minimum energy
and excursion remain 1.2 m/s and 8 cm. Fast sweep interpolation preserves the
blade's length along its rotational arc instead of collapsing its tip onto a
chord. A second lockout came from unbounded jitter/windup accumulation suppressing the
net-excursion ratio. Stroke onset now uses a bounded 120 ms / 32-sample history;
jitter still cannot qualify, but cannot poison the next deliberate slash. Final
replay: 1,413 powered samples / 48 distinct target-stroke contacts (289 intersecting
samples powered, 28 unpowered; previous build 18 powered / 299 unpowered).
This is **offline replay evidence, not headset validation of the update**.
Synthetic committed-source tests cover fast swings in four planes, jump/hitch
recovery while continuing to move, outer-arc contact, and misses beyond reach.
Native logs now include pose-reset/recovery counts.

A stereo screenshot with `[debug] bone_axes=1` confirmed +Y follows the visible
short sword. Read-only hero/weapon matrices likewise agreed with collider origin
and direction. Model bounds were inspected with an ignored Rusty Demon GR2 reader;
no game assets, third-party reader or captured player poses added to source.
Temporary bone axes have been turned off. Bow flight stays parked.
Cross-build and host melee/gameplay/combat/bow tests pass. Installed with game
confirmed closed 2026-10-09 02:21:55, DLL SHA256
`44d7f136ba2eda16b926f1e8a7dcd72778aa8f1a718b780e4a25e0b843597ea5`; backup `build-frame/backups/free-swing3-20261009-022155/`.
Runtime confirms the contact hooks ready at 02:22:03. Headset comparison requested;
owner reported much better but fast motion still misses. No commit/push.


**Owner feedback on the first free-swing build: "hit or miss at best."**
Revision 2 installed 2026-10-09 02:03:56, SHA256 `b4542fcba5c87fca39ad44752a31640d5cc6c9be80dcdb9c4c8d6433aa2fd752`; backup `build-frame/backups/free-swing2-20261009-020355/`. It refreshes native local-player
identity from the revalidated hero each active sample (not only old controller
aim traffic); rotation-aware reversal permits wrist-led repeated slashes; initial
acceleration geometry joins the first powered sweep; speed/excursion threshold
1.2 m/s and 8 cm; +2 cm weapon radius and +5 cm creature radius tolerance.
Ordinary get-hit/block animations no longer discard physical swings (death/town
still gated). Added intersection/stale/wall counts and tests for four consecutive
wrist-only reversals and onset-path inclusion. Owner saved/exited before deployment; revision 2 is launching for comparison.

**Installed 2026-10-09 01:54:19: free-swing contact melee, awaiting headset validation.**
DLL `f2d056b1a72f7742d4bf51cce5f4aa6163fcc7d8badf4ca4f55819656bed5971`;
backup `build-frame/backups/free-swing-20261009-015419/` holds deterministic-hit
baseline DLL/INI. Deployed with game closed and launched via run-proton.sh.
Host melee/gameplay/bow/combat tests and snapshot signature audit pass; cross-build
log `build-frame/build-contact-melee.log`. Runtime confirmed all hooks ready at
01:54:27. At 01:55 the server game/player/session identities matched and strong
swings queued successfully with no missing-server failures; no geometric contact
had yet occurred (nearest creature was ~15 game units away). No commit/push yet.
Owner rejected grip-to-arm + controller targeting. New `vr/melee.{cpp,h}` and
`steamvr/melee_contact.h` replace melee A pulses with swept weapon capsules and
exact contacted-target damage through native basic attack. No grip/trigger or
preselected target. Trigger fallback and bow/spell paths remain separate.

- `42FEB0` native basic-attack Do function owns the damage object and complete
  combat effects. TLS overrides `48FE20` GetTarget **only** at its two call sites
  inside that function (`42FFAA`, `43010A`). No path/selection fields are written.
- `48FE80` server lookup and normal GetTarget calls identify the local authoritative
  game using native type/id, unit+0xD8 backpointer and opaque SDK session identity.
  Requests run through `ThreadService::runOnGameThread`; incoming pointer identities
  are resolved there. Monster hash buckets at game+(0x4C6+i)*8, link unit+0x158.
- Native `44B720` result flags record success. `348650` melee range only for that
  scoped contact uses native `3502A0` line collision; physical geometry replaces
  old grid reach. `48E460` relationship, death/town/get-hit/block modes gate dispatch.
- Body capsules infer height from native MonStats2 `0975B0` pixHeight +0xD and
  footprint +8/+9. This is approximate artwork geometry, not mesh collision.
  Weapon category lengths use posed GunBone +Y; generic but needs physical tests.
- Physical room-space motion supplies energy; camera motion alone cannot hit.
  One target per stroke; quiet rearm or deliberate reversal; 150ms queue expiry;
  session/equip, menus, tracking resets and snap/recenter cancel pending contacts.
  Haptics mean accepted native hit flags, not proof of damage through immunity.
- **Native animation cadence is replaced by physical stroke cadence.** IAS scaling,
  per-model dimensions, offhand collisions and shield/parry behavior remain open.
- `steamvr/melee_contact_test.cpp` tests sweeps/misses, strength, jitter, frame rates,
  repeated contacts and cancellation. Gameplay tests assert no melee A pulses.

Live test at 01:57–01:58 recorded 8 geometric contacts, 8 accepted native
hits, 0 rejections and 4 missing-server failures. Runtime input shows grip=0,
trigger=0 with melee contact enabled. Player/session and MonStats2 footprint/height
fields were verified by read-only inspection. Owner collider-fit feedback pending.

Previous installed baseline (retain rollback details):

**Latest installed: deterministic outgoing hit test, 2026-10-09 01:30:35.**
Owner confirmed both right-grip and right-trigger melee work, then explicitly
requested removal of dice-roll misses for melee **and arrows/magic**, with strong
physical swings connecting on contact. Native hit probability is now overridden
for the local player's hits against creatures; full swept-blade contact is still
outstanding, and that distinction has been explained in commentary/test question.
DLL `b6edab6541cbb19784e90c594f54dadd5fcd5d7dcbfefff077558872524d0640`;
backup `build-frame/backups/contact-hits-20261009-013035/` contains the prior melee
DLL/INI. Game was closed before deployment. INI adds `[gameplay] contact_hits=1`.
Disable that setting to restore native miss rolls; motion_controls must be on.

New `vr/combat.{cpp,h}`, `vr/combat_rules.h`, `steamvr/combat_rules_test.cpp`:
- Native IsHitSuccessful `44BA60` wraps a TLS scope for the identified player.
  Seed getter `34A1E0` supplies a zero-valued scratch seed only at return address
  `44BD6D` within that eligible accuracy check; native minimum chance is 5%.
  The real seed is advanced normally once (low*0x6AC690C5+high) before scratch is
  supplied, preserving the following attacker damage/proc roll. Rest of the
  function runs normally, including Prevent Monster Heal on the accepted hit.
- `44B930` ApplyBlockOrDodge and `44EF70` ApplyDodge return no random block/dodge
  only for that player attacking a monster. Native collision/range/relation,
  damage/resistances/immunities, ammo/mana and timings still apply.
- All four prologues plus exact inline-roll bytes must match; no overrides until
  all hooks install. Full native owner type/id matching excludes enemies, pets,
  other players, PvP, scenery. Projectile collision code calls these functions
  with its native owner; spells without an accuracy roll keep their collision path.
- **SDK PlayerHandle is opaque, never a pointer** (handles.h explicitly says so).
  `HookAttackTarget`, after IsHeroUnit validation, publishes native id+freshness
  and opaque session token to combat. Read-only live inspection confirms id=1,
  native unit type=0 and matching SDK session. The older parked projectile-speed
  module incorrectly tries SDK handles as pointers, explaining its absent counts;
  this was discovered during combat work but NOT changed (arrow tuning parked).

Host rule tests, snapshot signature audit, build (`build-frame/build-contact-hits.log`)
and diff checks pass. At 01:30:43 runtime confirmed all hooks ready. At 01:32:57,
01:32:59 and 01:33:03: three sword melee checks, three accepted accuracy rolls,
three block bypasses, zero failures. Owner visual feedback and live ranged/spell
checks remain pending. Async question asks whether hits are consistent.
By 01:36 the session totals were 19 melee checks / 19 accepted rolls / 19 block
bypasses, zero failures, and no projectile checks yet. Game remains running.

Research files in ignored `build-frame/combat-reference/`: primary D2MOO source
and license (read for control-flow reference), native disassembly. Temporary
read-only extraction of monstats2/weapons tables and short-sword asset stays in
ignored game-assets. No game assets/stock executable/saves/inventory edited.
Physical contact needs a proper weapon sweep, creature bounds and exact target
dispatch; do not silently substitute an arbitrary hand-radius check and call it
mesh collision. All motion/combat work remains uncommitted.

**Current priority: generic melee across characters and melee weapons.** Owner
tested the flight build below: "somewhat aligned and not that much faster" and
explicitly parked arrows to work on melee. Latest session (01:03–01:06) has hook
startup messages but no flight creation/velocity counters. Speed changes remain
unverified; do not continue arrow tuning unless requested. Game was closed when
this melee work began.

New shared main-hand melee changes in `steamvr/gameplay.h` / `transport.cpp`:
weapon-category sensitivity (unarmed/short/one-handed/long), translation and
rotation recognition, net-excursion guard against rapid jitter, quiet ready
haptic, one request per stroke, and 250 ms aim latch toward the swept hand's
ground bearing. Laser direction no longer redirects a motion slash; trigger
fallback still uses live aim. Tracking orientation jumps require releasing grip.
New melee equipment resets palette selection to Attack; ordinary tracking/menu
interruptions preserve deliberate skill selection. Claws/orbs/javelins/throwing
weapons enter this generic path as well: A must be bound to basic Attack for
melee. This dispatches the assigned action, not an invented damage call.
`HookAttackPoint` skips long-range staff/thrown aim extension during motion melee.
It retains the original game's reach, target selection, timing and damage rules.
No character or rig is special-cased; no physical blade collision, confirmed-hit
haptic or independent left-hand attack yet. Owner wants a generic solution,
not one restricted to Nekro or a particular test weapon.

Strict host gesture tests pass, including all four profiles, slashes/chops/
thrusts at 36/72/120 Hz, direction latching, menus/equipment cancellation,
orientation jumps and sign-equivalent quaternions. Bow pose / projectile helper
regressions and cross-build pass (`build-frame/build-melee-generic.log`).
Installed and launched 2026-10-09 01:13:53, game confirmed closed first:
DLL SHA256 `cc4eb561f2275ff1a32f0cefabe8e6980302e9ca9b03283958c843f4aa1d478a`.
Backup `build-frame/backups/melee-generic-20261009-011353/` holds the prior
flight-test DLL/INI. INI and saves unchanged. Headset validation pending. Async
question asks the owner to try slash/chop/thrust with any melee weapon outside
town, right grip held, basic Attack on A. Check `Frame melee: swing requested`
and `Frame gameplay ready ... melee ready=... grip=... aiming=...`; these prove
gesture dispatch, not native damage. All motion changes remain uncommitted.
Runtime 01:15–01:16: owner switched from hunter's bow to short sword (`ssd`),
recognized as melee type 4. Ready pulses logged; a motion swing at 01:16:00.898
was followed by native pad A attack at 01:16:00.916. Trigger fallback attacks
also reached native hand-aim hooks. This establishes equipped-type detection,
gesture and native input delivery; target accuracy/damage/comfort await feedback.

**Previous flight test installed/launched 2026-10-09 01:03:05:**
`d60ed9893e83d68753b34e009bc4feb66f2884f1b1c810cfbba920723c357a21`.
Backup `build-frame/backups/bow-flight-20261009-010305/` contains the
owner-verified two-hand bow DLL and INI. User had quit after firing; the timed
F10 recorder produced an empty file. Do not claim a flight capture succeeded.
The combat log did capture those shots. Read-only extraction from the owner's
Steam archives revealed the flying arrow is a **static model with no skeleton**,
with `TrackReleasePointDefinitionComponent.transitionTime=0.5`. Extracted game
assets and the temporary D2RExtractor storage reader stay ignored in build-frame.

New `vr/projectiles.cpp` isolates flight from the working held-bow pose:
- `[gameplay] arrow_speed=2` scales native PATH velocity only within local-player
  basic-arrow creation on both client RVA `1B7760` and server `5371A0`. The path
  setter is `342AE0`; type/id, unit↔path backlink and owner checks fail closed.
  TLS creation scopes restore on exit and mask nested unrelated creation. Speed
  activates only when all three signatures/hooks match. Native lifetime remains,
  so range can increase. Elemental/skill arrows and other owners are unchanged.
- `arrow_direct_release=1` bypasses the half-second HD handoff by removing only
  that arrow entity's TrackReleasePoint component with native operation `70DC70`
  before initializer `771A10`. Full entity generation and sparse/packed storage
  membership validated. Original native arrow path/transform remains. This is a
  targeted **test**, not proof that every flight orientation issue is solved.
- Owner lookup `09A7B0`, component storage `6E3CD0`, registry `2677170`; expected
  bytes checked. No game tables, stock executable, assets, saves or inventory
  edited. Motion controls must be enabled for either change.

Strict host gesture/pose and new ownership/velocity tests pass. Cross-build
passes. Flight-test log confirms client/server velocity hooks and release
handoff hook ready at 01:03:14. Actual arrow events were not logged; owner
reported only partial improvement and parked this (see current priority above).
Look for `Frame bow flight: created client/server ... accelerated ... direct
handoffs ...` if investigation resumes. Both settings are live-reloadable for comparison.
Uncommitted motion source now includes projectiles.{cpp,h}, projectile_rules.h
and steamvr/projectile_rules_test.cpp. Do not publish local game assets/dumps.


**New priority after the eye-tracking commit:** motion gameplay. Owner requested
real bows/melee, VR potion handling and weapon/skill selection/casting, with
6DoF controllers, and selected right-handed controls. The first gesture-driven
native-combat prototype is implemented, built and installed; see
[steamvr/GAMEPLAY.md](steamvr/GAMEPLAY.md) for exact behavior, controls and limits.
This is not completed collision/physics combat. Native skill metadata, accurate
surface targeting, item meshes, counts/health/mana and physical collisions remain.

Latest installed **left-attachment correction, 2026-10-09**, game confirmed closed:
DLL SHA256 `00ed67676d2959dbe86ebdd422911687947807499c302b6c4dafeca9ba4e5378`.
Rollback DLL/INI: `build-frame/backups/bow-left-anchor-20261009-004022/` (this contains the failed two-anchor build).
Last owner-verified draw build: `build-frame/backups/bow-two-anchors-20261009-003455/d2rl-vrcam.dll`.
Only the DLL changed. Motion controls remain locally on; retained DLSS off,
centered projection, working labels and eye-tracked foveation. Installer defaults
motion controls to **0**. Motion source is uncommitted; published ZIP remains old.
No save edits or synthetic equipment were made.

Owner acquired/equipped hunter's bow (`hbw`) and arrows (`aqv`) on an offline
Amazon. Initial left-grip grab failed. The 00:01:59 build removed that prerequisite
and added trigger hysteresis, pre-squeeze entry, a larger grab region, arrow/
V-string hints, and diagnostics. Owner confirmed draw and shooting work, but
actual native bow pull/rumble happened after trigger release. Runtime captured
13 nocks / 12 ready / 12 release events, and native attack/aim pulses.

Read-only bone captures in ignored `build-frame/bow-poses-motion-20261009.json`
confirmed the native wind-up began after release. Current build adds
`vr/bow_pose.h`: string/limb/arrow deformation from runtime bind geometry,
constant string/limb lengths, draw amount tied to hand separation, snap-back on
release, hide arrow until nocked. `PoseMotionBow` validates the 13-bone rig's
names/hierarchy/bind data and **full parent entity == current hero entity**;
NPC/dropped bows must not be changed. Only the nine deforming matrices after
cog are written. Valid input is copied through a separate lock; skeleton workers
never take the transport mutex. Input older than 150 ms fails closed. Unsupported
rigs keep native animation and quad guides. Successful posing hides duplicate
quad string/arrow. Native timing/damage/ammo are unchanged; immediate projectile release is not solved.

New haptics check OpenXR return codes, stronger grab/release pulses, both hands,
and light draw-progress pulses before release. Correct Frame controller bindings
already include the feedback action. `frame_hmd` has no hand haptics by design;
do not mistake that headset binding for missing controller bindings.

Validation: strict host gameplay and bow-pose suites pass; cross-build succeeds
(`build-frame/build-bow-pose.log`). Captured hunter's bow bind geometry passes
all draw fractions; procedural full draw closely reproduces the observed native
limb-tip/string positions. Runtime capture and generated validation data stay
ignored, not bundled game assets. **Owner verified the 00:23 native pose/haptics: “yes it all works.”**
Then reported arrows flying sideways and slowly, requesting exact two-hand
anchors: right hand at arrow tail, left at bow, visible arrow along their vector.
The 00:34 build adds that visual pose: full attachment-to-head transform from
hero posing, composed with the bow entity's local transform; inverse maps both
hand anchors into cog space. String follows right hand, arrow rotates along
right-to-left vector, limb tips preserve string/limb lengths. Unreachable draws
are constrained. Invalid/stale attachment data falls back to normalized draw.
`anchored=1` in readiness logs confirms the new path while drawing. Synthetic
mirrored/scaled attachment and exact world-direction tests pass. Native aim uses
the same two-hand line and retains it for 400 ms after release (attack pulse
still 65 ms). Vertical projectile motion and speed remain native/ground-based.
The 00:34 two-anchor build failed the owner's visual test: draw disappeared and
arrow pointed right at nock. Root cause found in attachment selection: generic
`axesSide` follows **inventory slot**, which selected the right wrist, although
ECS world-matrix comparison confirms the actual bow hangs on L_weapon_attach
(error ~0.0002 units). Inverting the right-hand attachment cancels right-hand
pull motion! The latest correction publishes a dedicated `leftToHead` matrix
from L_weapon_attach and uses that for the two-anchor conversion. Built and
installed after game closed; the owner subsequently confirmed the held arrow and
string follow both hands (see measurements below).

Latest 00:40:22 build now has runtime geometric evidence: ignored read-only
`build-frame/bow-anchor-measure.py` measured 734 live draw samples, comparing
posed string/arrow bones through the actual item and left attachment transforms
to the tracked anchors. String-to-hand p50/p90 errors ~0 / 0.00326 m; arrow versus
hand-line p50/p90 angles 0.0335 / 0.4519 degrees. No `xrEndFrame` errors since this
restart. Haptic/nock/ready/release events continue. This checks draw geometry,
not visual comfort, native projectile direction or velocity. Owner confirmed the corrected draw follows both hands. Latest unresolved
request is specifically **flying arrow model angled across its flight path**
(not the held bow twisting; explicitly clarified). Flight path is roughly
straight. Arrow must also travel much faster. Plan a 2x native travel-speed
test without accelerating game time; velocity has NOT been changed yet.
Need identify the native flying-arrow asset/transform separately from held bow.
Game was saved/exited; relaunched current good nock build for inspection. Check `Frame gameplay ready` for `native=1` and draw
values while holding trigger; haptic acceptance does not prove felt feedback.

During 00:25–00:26 tests `native=1`, changing draw amounts and successful
both-hand OpenXR haptic submission are confirmed. Repeated `xrEndFrame -39`
(`XR_ERROR_POSE_INVALID`) correlate with the idle nock marker, clearing during
physical draw. The same errors existed in the 00:01 guide build. The 00:34 build omits that redundant marker once native bow posing is active
and normalizes/validates optional guide poses. Logs show zero frame submission
errors through the subsequent loaded-game bow test (00:36 onward).
Investigate before treating perceived arrow speed as solely native velocity;
no game-table speed/balance change has been made. User nevertheless confirms the live draw.

Other motion implementation: `steamvr/gameplay.h` recognizer, `transport.cpp`
OpenXR grip/aim/haptics, static atlas and stereo palette/bottle/guides, native
controller skill routing; `vr/gamecmd.cpp` timestamped native belt requests on
UI thread; `vr/vrcam.cpp::HandRay` uses motion aim. Right A's duplicate raw pad
mirror was removed so semantic no-target protection applies. Melee, potions,
skill selection and broader regression scenarios still need individual evidence.

Motion controls retain left stick walk/right snap/Left X inventory. Left Y
holds palette, right trigger selects eight B/X/Y/RB and LT-variant native skills
(no rebinding/names yet), attack mode or weapon swap. Grip over belt column
grabs a neutral numbered bottle guide; hold, raise to mouth, tip and dwell .25s
to request its normal belt command. Swing detection requires right grip and
quiet reset. Bow uses right trigger to nock within .28m, then at least .12m additional
draw, .28m total hand separation and release; no left grip is required. Select Melee/Bow for
gestures. Both tracked hands required. Physical collision, free 3D projectile
origins and pitch-sensitive terrain targeting are explicitly not implemented.

Older performance/visual evidence below remains useful but is superseded as
the immediate work priority by motion gameplay.

Latest performance evidence: after another competing Iron Signal workload was
stopped around 21:46, the owner reported much better movement with slight residual
bursts. The earlier 21:39 **“Smoother, bursts gone”** report was followed by more
contention from a new process. Check current host load, not just the original PID.
At 21:48 the owner enabled **Balanced DLSS** and reported distorted visuals and
a temporary pause. NGX parameter ABI and correction-size-limit bugs were repaired;
the **22:04:32** build logs correction submission for both eyes. The owner then
confirmed **“looks good”** with Balanced DLSS. They also reported subtle left/right
differences in emissive/other effects that may predate DLSS (details below).
Keep `[render] defer_gpu_wait=0`; the experimental synchronization change did
not explain the bursty motion. See the capture results and limitations below.
After the 22:42 installation, the centered-projection lighting trial received the owner report:
**“patches seem to be gone. Though the image is a bit softer.”** Keep the installed
`[render] symmetric_projection=1`. This supports the native light-bin off-axis
projection diagnosis; broader scene/clothing checks remain. The wider rendered
FOV at unchanged texture resolution explains an expected sharpness tradeoff.
After the 22:48 relaunch the owner turned DLSS off and reported **“much clearer.”**
Current visual baseline is therefore **DLSS off**, centered projection 1, fixed
foveation 1 and conservative synchronization 0. Do not restore Balanced on a
future relaunch; the earlier Balanced preference is superseded. Settings.json
confirms NVIDIA DLSS=0; the scene log switched from 0.580 to 1.000 at 22:49:51.
The comparison shows DLSS contributed to softness; do not attribute all blur
to the centered projection or treat this as proof the DLSS correction is flawless.
The user also reported **“Smoother”** for the 19:49 build on 2026-10-08, which
caches foveation command-list/pipeline state, defers redundant
VRS binding, batches diagnostic draw counts and fixes CPU accounting on Proton.
The user explicitly prioritized **performance first, creature labels second**.
The source checkpoint includes these fixes. The published v0.1.0 preview ZIP
still predates them; build current source or publish a new preview before telling
a friend the downloadable binary contains the lighting/label corrections.

1. **Residual performance hitches and CPU rendering overhead.** The logger's
   disk stalls and repeated foveation callback work have been reduced. Do not
   claim all remaining stalls are shader compilation.
2. **Broader creature-label regression checks.** After centered projection and
   current-eye health-panel draw ordering, the owner confirmed **“fixed :)”**.
   Both fixes are now enabled by default. Bosses, rapid target changes and a
   wider range of distances still deserve checks; no need to repeat failed
   anchor-only experiments. Details and limitations below.
3. **Validate Quest 3 + WiVRn**, or Windows + Virtual Desktop, using the friend's
   agent and real headset. Check runtime extensions before changing the transport.
4. Finish menu regressions, targeting/loot feedback, on-demand belt/health/mana,
   weapon/spell selection and optional melee/bow mechanics from TASKS.md.

## Implementation map

| Area | Files |
|---|---|
| Native OpenXR, eye copies, controllers, recenter, menu laser | `steamvr/transport.cpp`, `math.h`, `input.h` |
| Fixed foveation / D3D12 Tier 2 VRS | `steamvr/foveation.cpp`, `foveation_math.h`, `vrs_compat.h` |
| Buffered runtime logger | `steamvr/logging.cpp`, `logging.h`, `logging_test.cpp` |
| First-person camera, per-eye label hooks | `cleanroom/camera/d2rcam.cpp`, `mat4.h`, `game_layout.h` |
| Native UI panel toggles / game state | `vr/gamestate.cpp`, `vr/vrcam.cpp` |
| Body selection / checked scanning | `vr/skeletons.cpp`, `vr/probe.cpp`, `steamvr/hero_selection.h`, `memory.h` |
| Room/draw-distance and fog scans | `cleanroom/drawdist/drawdist.cpp` |
| MinGW/MSVC ReShade aggregate-return ABI adaptation | `external/reshade/include/reshade_api*.hpp` |
| Install/restore, Linux and Windows Steam wrappers | `steamvr/install.py`, `launch.sh`, `session.py`, `launch_windows.py` |
| Portable binary preview packaging | `steamvr/package.py`, `cmake/openxr-loader/` |

## Creature-label investigation

**Verified result:** the owner confirmed **“fixed :)”** after the corrected
draw-order build at 23:03. Centered projection and current-eye health-panel draw
ordering are enabled by default for native OpenXR and in the installer. DLSS
remains off. A subsequent 23:05:10 pair confirms both eye paint callbacks execute
after UI updates, but caught no creature, so it is not a quantitative post-fix
label-disparity measurement. Capture counters are reset to 0.

Earlier diagnosis: centered projection removed the large separation in the owner's
view, but they clarified **label depth seems off**. Capture **22:54:42**, pair 1,
shows a Zombie label centered at **X=2030 left / 2049 right** in 3840x2160 images:
the box shifts right while the creature shifts left. Corresponding native unit
anchor X values were 2037.20 left / 2021.59 right. Thus large apparent doubling
has improved while the remaining disparity still has the wrong sign.

A live comparison with only `stereo_labels=0` produced **“About the same.”**
Restored 1, reset capture_labels=0. Second local pair is **22:56:27**. This
does not implicate the unified-head correction as the source of residual depth.
Keep centered projection 1 and DLSS off through this investigation.

A fresh draw-order fix is built, now default `[hud] monster_draw_order=1`.
It reinstates the narrow deferred-health approach after read-only verification:
native PanelManager::Draw RVA **0x89F2E0** delegates to Widget::Draw **0x855B00**
(visibility and child traversal); it does not end the caller's UI render pass.
D2RLoader's vtable thunk is retained. Native panel-draw signature is checked in
addition to widget/tooltip signatures and both expected vtable entries. The
trial is scoped to the render thread's current stereo gameplay eye, queues no
widgets across traversals, and can be disabled live after installation. It is
independent of the anchor-correction setting. A capture logs health-panel paint
after native tooltip updates for each eye. No label textures/health values are
replaced. Local disassembly evidence is ignored `build-frame/{widget_draw,
panel-manager-native,panel-manager-loader,tooltip_draw}.asm`.

Host camera and deferred-order tests plus the cross build pass. New DLL SHA256:
`477d2b2d55f7440b297e36909c22e37ea486f9ede38c699740a6f6066d5c513b`.
Installed after Save & Exit at **23:00:55**, then relaunched. Backup:
`build-frame/backups/label-order-centered-20261008-230055`. Installed setting
monster_draw_order=1; centered projection 1, stereo_labels 1 and DLSS off retained.
Startup at 23:01:03 kept native paint order because the added PanelDraw signature
omitted its leading 0x40 prefix. No draw-order correction ran in that launch.
Corrected the byte pattern and verified all three complete signatures against
the live process (including the prefix), then rebuilt successfully. Corrected
DLL SHA256: `2eb6287946b1eeee02868cfb63c0baff936cc3625b10fd168f7d54100d1d7781`.
Installed after Save & Exit at **23:03:15**, backup
`build-frame/backups/label-order-signature-20261008-230315`.
Startup **23:03:23.269** confirms the hook installed; stereo gameplay resumed at
23:03:43. The owner then confirmed the depth fixed. Camera/queue logic is
unchanged; do not describe the first launch as a label-depth comparison.
The subsequent source build changes only missing-setting defaults to match the
already tested installed values. The running DLL need not be replaced for that.

Game build **3.3.93787**. Signature-guarded optional hooks:

- UnitToScreen RVA **0x76A7D0**, signature
  `4c 8b dc 55 56 57 41 57 49 8d 6b a8 48 81 ec 38 01 00 00`;
  `bool(context, unit, float output[2], uint8_t above)`.
- Camera::WorldToScreen RVA **0xED5EB0**, signature
  `48 8b c4 48 89 58 10 57 48 81 ec 40 01 00 00`;
  `bool(camera, float point[3], float output[2])`.

The native unit path resolves the HD model transform/bounds, projects head and
feet, then combines **projected foot X with projected head Y**. Inline camera
projection rebuilding can bypass the normal projection hook. The first fix used
the exact eye scene matrices in WorldToScreen, scoped only to unit labels:
native/exact projections differed by under one pixel and labels still doubled.
The second fix captures original head/base world points, combines base X/Z with
head Y into one world anchor, projects once per eye, and replaces output XY.
It corrected horizontal placement by roughly 20–40 px but **still doubles**.

`[hud] stereo_labels=1` enables these current experimental hooks. Behind/near/
invalid points are culled. Menu and shadow fitting projections are unaffected.
Full viewport dimensions, asymmetric FOV, row-vector / row-major / right-handed
-Z / reverse-Z math are covered by tests. Passing them is not visual proof.

F8 requests the next complete stereo gameplay pair through ReShade screenshot
capture; both images share a `Frame-pair-N` suffix with left/right labels. The
log records bounded unit-anchor data, eye FOV and pose for that pair. A remote
agent can instead increment positive `[debug] capture_pair` in the installed
INI while the game runs. A request is consumed only during active stereo
gameplay. A positive `[debug] capture_labels` arms a one-shot capture after a
visible native above-unit label is observed; changing that counter rearms it.
Readback causes a diagnostic pause: exclude that pair from benchmarks.
Screenshots contain game imagery and must remain local/ignored.

The automatic capture at 20:13:51 caught **Kashya**, one clear label per eye.
At **20:16:28/29**, pair 2 caught **Fallen Shaman**: the creature shifts left in
the right eye, but its health box shifts right. UnitToScreen returns native X
2429.48 (left) / 1779.95 (right) on a 3840-wide viewport. The rendered box instead
tracks the preceding eye's position. Each image contains one box, not two.

Code evidence: `TooltipsPanel::Draw` **0x880550** calls the target-name routine
**0xC0420** at 0x880AA5/0x880AE3/0x880B0C. That calls helper **0xCE850** and
monster information routine **0xCEA50**, which sends a UI event carrying the
current anchor. `MonsterHealthPanel::OnEvent` **0x1F8E60** updates its
`TargetAttached` and `ScreenAnchored` children via **0x1F84E0**. The attached
child receives the packed anchor at event payload +0x30 through **0x857050**.
The independent health panel can already have painted before TooltipsPanel.

The **20:33 draw-order prototype was withdrawn**, not validated. It patched
PanelManager paint (0x1D79CE8, original D2RLoader thunk 0x3E2B7F6) and
MonsterHealthPanel paint (0x1CDF3D0, generic 0x855B00), queuing the health widget
until the root traversal returned. A CPU queue test and startup signature checks
passed, but do not establish renderer-state or ABI correctness. Source and DLL
are archived locally in `build-frame/backups/stereo-regression-20261008-205705`;
source was removed from the working tree. Do not reinstall that DLL blindly.
The preceding DLL/effect were restored from
`build-frame/backups/monster-ui-order-20261008-203340` at 21:00.

The owner reported broken stereo, incorrect head pivot and third-person view.
Restoring that DLL did **not** fix it. The log shows camera mode changed to
third person at **20:39:05.943**, persisted as `[mode] vr_mode=2`, and reloaded
at 20:53 and 21:00. At **21:04:35**, changing only `vr_mode=4` restored first
person. The owner confirmed **“Both are normal now”** for camera pivot and
stereo convergence, with ReShade effects still enabled. The input that triggered
the mode change is unknown; F2 and shared legacy view actions both reach SetView.
Only the first-person camera uses native OpenXR eye poses; other modes had
submitted incompatible camera images with the headset poses/FOV.

A native-only guard now repairs saved VR modes to first person and rejects
legacy mode-switch shortcuts, preserving flat mode and the FlatVR build.
The guard DLL was installed after Save & Exit at **21:09:14** and relaunched.
Backup: `build-frame/backups/camera-guard-20261008-210914`. Live regression
checks passed: writing `vr_mode=2` was repaired automatically at 21:09:50, and
F2 was ignored at 21:09:51 with the saved mode still 4. Startup confirmed native
OpenXR and compiled ReShade effects. DLL SHA256:
`78dcb5a53b688310a26e442812997a071b63b0b8093f51efeb36971e7b5ea140`.
The installer explicitly writes mode order 2.
No ReShade-effects A/B was necessary after this controlled mode repair.
Scroll Lock remains configured as the global ReShade effects toggle but was
not pressed. Debug capture counters are reset to zero.

At that earlier stage creature labels remained unresolved. The mode error means the withdrawn build
was not evaluated in the intended camera mode; it neither proves nor disproves
the draw-order approach. Instrument health widget paint/event ordering in first
person before reintroducing it, including validation of UI renderer state.

UnitToScreen callers found: 0xCE877, 0x198E90, 0x198FCE, 0x1FABDF (items),
0x6D7B15, 0x1519482, 0x1519875. Helper 0xCE850 applies scaled UI offsets.
The ReShade UI trace (`vr/uitrace.cpp`, Ctrl+F10) can dump one GPU frame and raw
targets. It causes a capture pause; do not mix it into a performance comparison.
UI target is R16G16B16A16_FLOAT with **inverse alpha**, cleared black/alpha 1.
Do not infer duplication merely from different left/right pixel positions:
the headset's asymmetric optical centers produce large legitimate differences.

## Performance findings

A bounded profiler was installed at **21:19** (backup
`build-frame/backups/frame-profile-20261008-211901`). It leaves all graphics
settings unchanged. Set `[debug] profile_capture`
to a new positive integer for 360 active stereo pairs; reset to zero afterwards.
`steamvr/profile_report.py` parses the log and exports CSV. See README for
measurement limits: main render thread only, inclusive callback scopes (hot
callbacks sampled 1/64), GPU scene queue spans include CPU starvation. No
worker-thread attribution is provided by these callback measurements.

The first captures contain 360 valid GPU samples each. Sequence 1 was standing
still; sequence 3 was genuine movement (65 distinct logged hero positions).
Sequence 2, misleadingly saved as `profile-walking.csv`, was stationary: do not
use it as a walking benchmark. Local artifacts are under ignored `build-frame/`.

| Measurement (ms per stereo pair unless stated) | Standing, sequence 1 | Moving, sequence 3 |
|---|---:|---:|
| Mean full cycle | 27.027 | 48.474 |
| 95th percentile cycle | 34.823 | 80.985 |
| Both eye passes, elapsed | 16.938 | 34.913 |
| Main drawing thread CPU in both passes | 16.306 | 25.000 |
| GPU completion wait | 3.721 | 3.534 |
| ReShade effects, both eyes | 1.416 | 1.454 |
| Eye copies, both eyes | 0.046 | 0.048 |

These are different scene/motion workloads, not an optimization A/B. Eye drawing
dominates, while copies and effects are comparatively small. Sampled main-thread
VRS callbacks total roughly 0.6–0.9 ms per pair. The rest is not automatically
engine-only cost: uninstrumented mod work and worker threads remain possible.
The GPU scene timestamp span includes queue starvation and is not GPU busy time.
No half-second stall occurred in these bounded captures; the separate 581 ms
pre-render stall still needs attribution. Read-only process sampling is saved as
`frame-profile-process.csv`; kernel perf stack sampling was unavailable under the
current `perf_event_paranoid=4` setting. No global permissions were changed.

A synchronization comparison DLL was installed after Save & Exit at **21:27:59**:
SHA256 `1727c9f4fe30f8f7571605797e85dc38a5175e9d35c3e8d0205bf889bd092f19`.
Backup: `build-frame/backups/deferred-wait-20261008-212759`.
`[render] defer_gpu_wait=1` submits image copies/state transitions on the D3D12
queue bound to OpenXR and checks each copy allocator's previous fence before
reuse, instead of unconditionally blocking before image release. Shutdown still
waits for resource completion. The runtime orders access on the bound queue;
Proton's bridge drains pending submissions before its release barriers. Source
default remains **0**. Active GPU timestamp profiling deliberately retains the
old completion wait; use `[debug] profile_gpu=0` for this synchronization A/B.

Sequences 10–13 first compared 0/1/0/1. The owner reported moving at the start,
confirmed in sequence 10, so the test was repeated as **1/0/1/0**, sequences
20–23, from 21:31:25 to about 21:32:30. Each sequence contains 360 pairs, with
the hero remaining at `(8740.6, 11277.0)`. Viewing direction varied, particularly
in the conservative runs; do not present the aggregate difference as a controlled
percentage speedup.

| Repeat captures, 720 pairs per mode | Conservative (21, 23) | Deferred (20, 22) |
|---|---:|---:|
| Mean cycle, ms | 39.671 | 35.831 |
| Median cycle, ms | 36.451 | 32.123 |
| 95th percentile cycle, ms | 63.505 | 59.101 |
| Mean XR end, ms | 5.721 | 3.846 |
| Mean GPU completion wait, ms | 3.418 | 0.000 |
| Mean runtime submission, ms | 1.705 | 3.082 |

Some removed wait reappears in runtime submission; the full 3.4 ms is not a net
saving. The combined XR end stage is about 1.9 ms shorter in this sample. This
supports a modest improvement, not a solution to rendering cost or long hitches.
No new XR/D3D12 failure or copy timeout was logged during the comparisons.
At 21:32 the owner tested `defer_gpu_wait=1` and reported bursty slow/fast movement
and Left X not opening inventory. The setting was restored to **0** live; the
owner said movement was unchanged. Do not treat deferred waits as a validated
improvement or enable them by default. Subsequent inspection found an unrelated
Iron Signal test consuming roughly **18 logical CPUs on this 32-thread host**,
another substantial confound for the comparisons. Its executable was
`/tmp/iron-x11-target/release/iron-signal-desktop`, PID 3286587. The owner explicitly
authorized a 45-second pause/resume comparison; results follow below.
No graphics settings, effects, saves or game executables were changed for the test.

The Iron Signal workload began at **21:23:45**, just before the synchronization
comparisons. At 21:37:33 it was paused with an independent 45-second resume
watchdog. The owner also asked the other agent to stop its performance work.
At 21:38:18 the pause ended; the original process subsequently exited. No process
was left stopped. New/lighter Iron Signal and compiler jobs were still observed,
so sequence 31 is **not** a repeat with the same heavy workload resumed.

| Capture (360 pairs each, conservative sync) | Paused, sequence 30 | Heavy process gone, sequence 31 |
|---|---:|---:|
| Logged hero positions | 1 (stationary) | 42 (walking) |
| Mean cycle, ms | 33.519 | 29.400 |
| 95th percentile, ms | 37.923 | 37.120 |
| Maximum, ms | 55.213 | 41.831 |

The owner then confirmed **“Smoother, bursts gone.”** CPU contention was a major
contributor to this regression. Scene/motion and competing workloads changed,
so do not derive a precise percentage gain. Local CSVs:
`build-frame/profile-contention-30.csv` and `profile-contention-31.csv`.
All capture counters are reset to zero. Check live competing CPU/GPU workloads
before any future VR benchmark; request approval before pausing another project.

The inventory regression was desktop focus: `_NET_ACTIVE_WINDOW` was a terminal,
and native Left X was unnecessarily gated on Win32 foreground focus, despite the
XR session remaining focused. Restoring the game window using `xdotool
windowactivate --sync` restored inventory; owner confirmed it and logs show
successful open/close cycles. `wmctrl -ia` alone did not actually focus this window.
The fix was installed at **21:40:18**, after the owner saved and exited, then
D2R was relaunched. Backup: `build-frame/backups/menu-focus-20261008-214018`.
Current DLL SHA256:
`357a5a0aa18f5c37dba624423ef53c9cc53ddee38992ae15c8bfbb178961a64e`.
It removes desktop-focus gating only from the native D2R menu action.
Explicit menu actions/clicks also try to focus the game;
OS mouse/keyboard injection still requires actual foreground focus, and passive
laser hover never steals focus. Build and existing input/math checks pass;
the intentional terminal-focus test confirmed native inventory opens/closes.
Windows/Proton rejected `SetForegroundWindow` in this test, so laser input was
correctly withheld while the terminal retained focus. The owner confirmed the
behavior and explicitly said **not to pursue a further fix**: requiring the game
window to stay focused is acceptable. INSTALL/README now state that requirement.
The game window was restored to the foreground after the test. Respect this
scope decision; do not add a focus broker or another workaround unasked.
Later at 21:40, different Iron Signal runtime/test processes again consumed
substantial CPU. Do not assume the host is idle because the original PID exited;
the owner has asked the other agent to stop performance work. No additional
processes were paused.

At 21:46 the heavy workload was gone and the owner reported much better movement,
with a slight residual burst. Temporary D2R-only CPU affinity trials (sequences
40–43, one 5950X L3 group versus original affinity) were confounded by changing
movement and the other workload stopping. Do not claim an affinity gain. All
original thread affinities were restored; no launch affinity change was made.
Sequence 44 started later than intended, after DLSS was enabled, and is saved as
`build-frame/profile-dlss-broken-44.csv`, not an affinity comparison. It includes
a 22.7-second first-eye CPU stall ending at the first DLSS correction shader
build; steady-state median is 30.75 ms, p95 34.53, but the mean includes that
startup pause. Do not compare this different scene directly with DLSS-off runs.

### DLSS compatibility trial, 21:55

The owner enabled Balanced through D2R's options (`NVIDIA DLSS: 3`); do not
silently overwrite it while the game runs. Per-eye NGX features were created,
but the motion-vector correction logged unreadable Depth/MotionVectors inputs.
`vr/dlss_mv.cpp` redeclared the SDK's overloaded virtual interface. MSVC reverses
each Set/Get overload group; our MinGW build does not. The old D3D12 resource
getter dispatched to byte offset 112 instead of 72, and its setter to 48 instead
of 8. `vr/ngx_parameter.h` now supplies unique virtual names in MSVC slot order
with nonvirtual typed wrappers. `steamvr/ngx_abi_test.py` compiles all 17 calls
against an independent SDK-order declaration with both Windows compiler targets;
the fixed code matches MSVC, and the old MinGW layout fails all 16 overloaded
calls. No external C parameter wrappers are exported by this `_nvngx.dll`.

Built and installed SHA256
`0a9df2d19fdff8a57f065567df5ec142a63752f6bc936e85dbbd42d9f0a1dd3d`.
Backup: `build-frame/backups/dlss-abi-20261008-215509`. Relaunched offline VR;
Balanced retained, `defer_gpu_wait=0`, profiling idle (`profile_capture=0`).
New logs report NGX input failure result codes or a correction submission for
each eye. This proves neither correct vector math nor acceptable visuals until
checked in the headset. Near-parallax correction remains upstream-default off.
The original shader still compiles synchronously on first DLSS use; the previous
session used the private Microsoft compiler and paused about 23 seconds here.
Installer default remains DLSS off until the trial is validated.

The owner restarted SteamVR during the first launch; relaunched at 21:57 with
Frame reconnected. At 21:58:18 the fixed ABI read valid resources: depth
3840x2160 R16_UNORM, vectors 3840x2160 R16G16_FLOAT, scene 2227x1253, MV scale
-2227/-1253. The first shader build again stalled about 22.5 seconds. Correction
still did not submit because the diagnostic statistics limit (900,000 bytes)
was incorrectly applied to mode 2, which never writes those statistics. The
next build moves that guard to the diagnostic path only. After the owner saved
and exited, installed at **22:04:32**, SHA256
`d8c298c663e10f261272a407af4bb4498bd535807529a5eb70d0b4048f1dcacd`.
Backup: `build-frame/backups/dlss-size-20261008-220432`. Relaunched with Balanced
retained. At **22:05:39** the log confirms valid inputs and correction submission
for **both eye 0 and eye 1**, scene 2227x1253. The initial shader build stalled
22.3 seconds again. The owner subsequently confirmed **“looks good.”** This is a separate
bug from the ABI.
A stereo capture at 21:59:14 is in the game directory (`Frame-pair-1-left/right`);
it is not a performance sample. `capture_pair` was reset to 0 afterwards.

Post-compilation sequence 50 (`build-frame/profile-dlss-corrected-50.csv`) has
360 pairs: mean cycle **25.946 ms**, median 23.282, p95 33.643, max 35.431.
Both eye passes total 16.832 ms wall / 15.973 ms render-thread CPU; XR wait
3.102 ms and GPU fence 3.477 ms per pair. No GPU timestamp capture was enabled.
This is a short current-scene observation, not a matched DLSS on/off benchmark.
`profile_capture`, `capture_pair` and `capture_labels` are all 0 again.

### DLSS correction specialization, 22:30

Separated the production correction shader from the diagnostic statistics shader
at compile time in `vr/dlss_mv.cpp`. Pipelines are created on demand and retained
independently, so normal mode 2 never compiles the diagnostic reduction. The old
uniform branch reserved 27 KiB of group-shared memory even for correction-only
work. The new correction declares no group-shared storage or group barriers.

An isolated Microsoft SDK compiler probe under a separate Wine prefix measured
**33 ms / 4,632 bytes** for correction versus **22,880 ms / 52,028 bytes** for the
old combined shader. Diagnostics still compile in about 22.7 seconds, only when
explicitly selected. Compiling the old shader with its correction branch fixed
true produces **exactly the same shader instructions** as the new production
shader. Artifacts: ignored `build-frame/dlss-shader-probe.log` and
`dlss-{old-0,new-0,new-1,reference-0}.asm`. The cross build passed. This establishes
shader compilation improvement, not a measured in-game FPS gain.

Installed after Save & Exit at **22:30:59**, backup
`build-frame/backups/dlss-specialized-20261008-223059`. DLL SHA256:
`4471de6d4b0de9c9d153bbeccf7dbbf941fdcacf427fb224dc2ef3f92bc457dd`.
The owner had left DLSS off after the diagnostic comparison; restored only that
setting to their original Balanced (3), then relaunched using `--frame-vr`.
Live validation at **22:31:57.798** logged the 4,632-byte correction pipeline
ready in **67 ms**, followed by correction submission for both eyes at
2227x1253. The initial pair was 95.8 ms rather than the earlier roughly 23-second
stall. This confirms the startup fix; steady-state FPS has not been compared.

### Distant stereo lighting patches (initial centered-projection check passed)

The owner clarified: glow/emitted light around waypoints, portals and lamps,
also highlights on clothing, show different square patches in each eye and
flicker with head turns. Flames themselves look fine. The fault disappears
close up and is visible at a distance. Controlled observations this session:

- Added sky/fog/ceiling disabled together: **still flickers**. Restored all 1.
- Fixed foveation temporarily off: owner still described the different square
  patches. Restored 1. The later capture was after its automatic restore, so do
  not describe the screenshots as foveation off.
- Owner switched DLSS off in game at the same distance: **still present**.
- Temporarily suppressed bloom upsample pipeline `25811850` in the then-running
  process (PS hash `ffda83db6037f452`): **still visible**. Restored skip_pipeline=0
  after the response; watchdog finished. This reduced soft glow, not all bloom.

Stereo pair at **22:16:14** (left/right PNGs in the game directory) and a native
GPU trace at **22:16:24** capture the affected distance. Trace and 23 decoded
render targets are in ignored `build-frame/lighting-capture-20261008-221614/`;
raw targets remain beside the INI. Targets are overwritten at successive uses,
so one final resource dump does not establish which draw introduced an artifact.
The trace records command-list recording order, not global GPU execution order.
The centered-projection experiment below is installed and passed the owner's
initial waypoint check; wider validation is still needed.

Read-only native inspection found `LightBinPass::Render` at RVA `0xFEA150`,
called by lighting task `0xE02920`; the light bins use a 32x20 grid. This is a
candidate for square lighting artifacts, **not a confirmed cause**. Renderer
singleton is `*(base + 0x34F6480)`; its LightBinPass is +0x1BD58, its clip-plane
buffer member +0x78, and global-frame constant-buffer CPU pointer +0x11BB8
(size 0x5A0). Setup at 0xFE9760, compute shaders `light_bin_c` and
`light_bin_depth_bounds_c`. Global constants built at 0xE3B590. Local assembly
notes are ignored `build-frame/lighting-*.asm`; do not publish raw game code.

Further read-only shader inspection found a concrete projection incompatibility:
`light_bin_c` reconstructs tile rays using only inverse-projection diagonal
CB3[17].x / CB3[18].y, and projects light bounds using CB3[13].x / CB3[14].y.
It never includes the off-axis projection terms. Frame's captured FOVs have
substantial horizontal and vertical asymmetry. Local-only disassemblies are
`build-frame/native-light-bin.asm` and `native-light-bounds.asm`. Shader asset
handles are LightBinPass +0x88/+0x98; object at asset +0x40, DXBC pointer at object
+0x38. This is evidence of an incompatibility, **not yet proof of the visual cause**.

A live `[render] symmetric_projection=1` mode is installed and now the default.
Once per successful XR LocateViews, it encloses both eyes'
recommended FOVs in a common centered frustum. It leaves poses/IPD/head pivot
unchanged and uses that same FOV for projection, VRS center, captures and
xrEndFrame submission. OpenXR supports submitted FOVs differing from the
recommended ones. At equal texture dimensions it loses some angular resolution;
it is a compatibility test, not a performance optimization. Revert live with 0.
Host math tests cover actual Frame FOV containment, diagonal-only ray
reconstruction, compositor mapping and stereo world-anchor convergence under
both projection modes. Source/installer defaults are 1 following the owner's
successful waypoint and subsequent creature-label checks; other headsets remain untested.
Cross build and host math tests passed. Installed after Save & Exit at
**22:42:35**, then relaunched. Backup:
`build-frame/backups/centered-lighting-20261008-224235`. DLL SHA256:
`1dccb3ff0f3e8e8337d91b8e3fc5689668a7c8e9e1ef4701663db46166b79806`.
Only the DLL and this one INI setting changed; Balanced DLSS and prior diagnostics
defaults were preserved. The live INI has legacy malformed header/comment lines,
so strict Python ConfigParser cannot read it; use section-scoped minimal edits.
Runtime **22:42:54.681** confirms symmetric FOV `[-1.03239, +1.03239,
+1.04476, -1.04476]`. Offline gameplay resumed at 22:43:03; both DLSS eye
corrections submitted successfully and the correction shader compiled in 35 ms.
The eye projection log has proj[8]=0 as intended.
Stereo capture **22:44:18** (Frame-pair-1 left/right PNGs in game directory)
confirms both eyes use the centered FOV with distinct positions about 6 cm apart.
The individual images show a coherent first-person scene facing the waypoint;
they cannot establish whether the moving-light artifact is gone. Capture request
reset to 0. The owner's subsequent response was **“patches seem to be gone.
Though the image is a bit softer.”** No new stereo/camera failure was reported.
Keep centered projection enabled locally. Do not compensate by increasing
render resolution or DLSS quality without considering the performance priority.
A more targeted light-bin correction could retain asymmetric projection and
pixel density, but has not been implemented or validated.
The subsequent DLSS-off visual check was much clearer. Recent unprofiled samples
at 22:50:03–22:50:33 measured 30.0–36.7 pairs/s and 17.0–21.2 ms for both eye
draws combined. These were moving/unmatched scenes, not a DLSS performance A/B;
do not infer a percentage performance cost from them. No graphics setting was
changed by the agent after the owner's DLSS-off choice.

### Standby/reconnect crash after lighting trial (parked by owner)

After Frame woke/reconnected at **22:45:24**, the kernel logged NVIDIA **Xid 32**
on RTX 5090 PCI 0c:00 in `vrserver` at **22:45:25**. SteamVR then failed to acquire
backbuffers; D2R's XR submit blocked for roughly 10 seconds repeatedly. SteamVR's
connection watchdog aborted at **22:46:06**, followed by a compositor segfault
at 22:46:07. D2R's 22:46:08 shutdown access violation at module RVA 0x530ED
matches its pre-existing 20:33:23 report. This establishes ordering, not the
ultimate cause or proof that the new projection is unrelated. NVIDIA describes
Xid 32 as a command-stream/DMA fault; do not diagnose hardware failure from this
single event: https://docs.nvidia.com/deploy/xid-errors/analyzing-xid-catalog.html

The owner confirmed it happened after waking/reconnecting, then explicitly said
**ignore this issue for now and focus on visuals**, and requested another launch.
No crash-related code, driver or runtime settings changed. SteamVR restarted
successfully; D2R relaunched with the same lighting fix and Balanced. The XR
session reached focused state at 22:48:57 and stereo gameplay resumed at
22:49:05 with automatic headset calibration. The owner is evaluating visuals.
Evidence preserved locally under
`build-frame/backups/crash-centered-20261008-224642/`: kernel/GPU records,
SteamVR logs, D2R/ReShade/Proton logs and minidumps. Do not upload raw dumps.

`HookDrawGameScreen` holds one headset pose per pair, right_dt_ms=0.01 preserves
particles. Native frame counters/temporal histories and camera-derived lighting
constants still deserve investigation. Do not zero dt or force identical
view-dependent highlights. The negative sky/fog test makes ReShade Timer changes
a poor first candidate. Original sync stays defer_gpu_wait=0; camera vr_mode=4.

Owner additionally reports a fuzzy, ghosted, vibrating desktop mirror. Each
native Present alternates eyes; this can explain mirror vibration/doubling but
not automatically all fuzziness. Individual eye captures at **22:33:06** do not
show large duplicated scene edges; a partially open/transitioning UI is present.
One still pair cannot rule out temporal reconstruction artifacts.

At 21:33:34 another 614 ms cycle contained a **565 ms pre-render gap**. It followed
`InputMethodChanged` and panel-layout changes. This is a clue to UI/input-layout
rebuilding, not proof of cause, and is separate from the usual 80–100 ms eye-draw
spikes. Do not blame shader compilation without more evidence.

Next performance work should attribute engine scene preparation and worker-thread
cost within each eye draw, then investigate reuse of genuinely eye-independent
work. Preserve separate eye cameras, culling, UI and resource lifetimes. Avoid
reducing image quality or removing effects based only on low spot GPU utilization.


The user also reports desktop mirror flicker. `HookDrawGameScreen` runs a full
left/right pair; each native Present displays that eye's backbuffer. The XR
capture is copied separately after effects; there is no third scene render for
the monitor. The ReShade addon `present`/`reshade_present` callbacks return void
and cannot cancel OS presentation. A stable one-eye mirror would require an
extra copy with the current path. No mirror change has been made. Investigate
safe optional display suppression and measure it separately; do not promise
an FPS gain or minimize the game and disrupt focus/input. Recent Present spans
are roughly 0.8–1.1 ms per eye, including more than display-only cost.


Old SDK runtime logging forced synchronous `fsync()` on the game main thread.
Live `/proc/PID/syscall` samples identified syscall 74 and the exact descriptor
path: **`d2rloader/logs/d2r-vr-vrcam.log`**. Several 540–784 ms pauses had repeated
ext4 journal/page waits, no physical disk reads, and almost all time before
stereo drawing. This was our diagnostic logger, not demonstrated asset loading.

`steamvr/logging.cpp` now queues up to 1024 records of 640 characters and writes
worker batches about every 250 ms, ordinary append, **no forced flushes**. Runtime
logs moved to `d2r-vr-frame.log`; rare SDK startup messages remain in the old file.
Queue overflow drops diagnostic messages instead of blocking gameplay on disk.
Concurrent producer/order/drain test passes under installed Proton.

After deployment the user confirmed improved hitches. 599 main-thread samples
showed no journal/page waits or blocked log file paths. Walking timing windows
had maxima around 38–60 ms; one **475 ms** cycle remained, about 445 ms before
stereo draws. Frame-pair throughput varies by scene (roughly 30–53 pairs/s in
observed windows), not a controlled benchmark or headset refresh rate.

Quiet Proton profile retains Wine errors and VKD3D warnings while dropping
exception/unwind tracing (previously >38,000 lines/second). Do not clear shader
caches. Existing VKD3D cache had about 1820 entries plus additions. Most recorded
hitches had no tracked pipeline creation, but deferred driver/compiler work can
escape those callbacks. Shader compilation remains a candidate, not proven cause.
Native height fog avoids repeated mod fog scans; video preset uses 80% resolution
and medium AO/shadows/atmospherics. ReShade sky/fog compilation was subsequently resolved with the private compiler below.

The 19:49 walking test measured about 42–45 stereo pairs/s in several windows,
with the drawing thread using CPU for 94–97% of its render-pass wall time.
RTX 5090 utilization had a median near 43%. Scene variation prevents claiming a
precise percentage gain; user feedback was “Smoother”. A remaining 504.7 ms cycle
spent 474.1 ms before stereo drawing, concurrent with roughly 27 MB of cached
file reads and a busy main thread. Asset processing is suspected, not proven.
Physical disk reads did not increase in the sampled interval.

Local evidence: `build-frame/foveation-cpu-process-samples.csv`,
`build-frame/foveation-cpu-gpu.csv`, backup
`build-frame/backups/foveation-cpu-20261008-194900`. A continuously busy worker
and ReShade's persistent “Compiling (1 effects remaining)” prompted compiler
investigation. An optional compiler can be preloaded from
`GAME/d2rloader/runtime/d3dcompiler_47.dll` before ReShade, only under the mod.
Do not use the old Windows 8.1/Winetricks compiler: it rejects `ps_5_1`/`vs_5_1`.
The Microsoft Windows SDK 10.0.26100.9169 x64 redist compiler passes a `ps_5_1`
probe on Proton. The full effect compiled in 10.56 seconds; subsequent samples
showed no continuously busy compiler worker. Sky rendering now works. This
adds previously missing effects, so do not treat it as a quality-matched FPS test.
Unfilled ceiling texture slots now use an empty source; the existing procedural
stone fallback remains and ReShade no longer reports missing optional textures.
The prefix's system compiler and stock executable remain unchanged.

## Foveation status

This checkpoint adds an opt-in **eye-tracked VRS prototype** after `b96718a`.
Installed October 8 at **23:18:47**, DLL SHA256
`981e4a19898316b02a729ebc97cbb3ccec8415ea2b0b12ba5cdf3283e8c19875`.
Backup: `build-frame/backups/gaze-pilot-20261008-231847/`. D2R was closed before
replacement; launched normally via `steamvr/run-proton.sh`, offline Nekro.
This supersedes the installed-build information earlier in this document.

Runtime evidence: 23:19:07 `supportsEyeGazeInteraction` succeeded, but the gaze
action remained inactive. The vrlink driver defaults
`shareEyeTrackingData=false`. The owner explicitly approved enabling SteamVR's
**Share eye tracking data to other apps on this PC** setting, including its
general PC-app scope. Set through the running `IVRSettings_003` API, read-back
false -> true, at ~23:21:53. **Do not request this permission again.**
OSC remains off. No headset restart was needed. The next sample reports
171 usable gaze pairs, 1 recovery pair, 2 inactive; the complete shared 9x9 bank
for 3840x2160 took 16 ms to prepare once. Reduced-shading draw counts are nonzero.
This confirms gaze input/VRS use, not a measured speedup or headset quality pass.

Implementation: `steamvr/gaze.h`, `gaze_test.cpp`, optional extension/system
query/action/space in transport, bounded immutable gaze banks in foveation.
`eye_tracked_foveation=1` must be set before launch; `fixed_foveation=1` is still
the VRS master switch. `gaze_foveation_inner=.35` plus a .177 grid quantization
margin; no camera or projection changes. Invalid/inactive/untimed/stale gaze
uses full quality; recovery and large jumps get one full-quality pair. Gaze
direction is projected separately using each rendered eye's rotation/frustum.
No measured fixation depth; generous margin covers close-object parallax.
The last gaze position is never held after tracking loss. Routine logs contain
validity counts only, not gaze coordinates/trajectories. Bank cache is capped at
four viewport/radius configurations; overflow safely shades at full quality.
Default installer remains `eye_tracked_foveation=0`, including Quest 3.

Cross build, host gaze/protected-region and existing camera-math tests, installer
tests and diff checks passed. The standalone probe now has `OUTPUT --capabilities`
to query support without a graphics session. The isolated shader-test Wine
prefix cannot load WineOpenXR (126); **the actual game's Proton session is the
verified capability/sample source**, not that failed standalone probe attempt.
The owner then confirmed **"Clear and stable"** while looking with their eyes,
turning their head and checking creature labels, followed by **"everything seems
good"**. A stationary gaze/full/gaze comparison captured 360 pairs per mode, all
with valid GPU timestamps. Gaze sampling stayed active even in full-quality
mode; only the VRS master switch changed. DLSS stayed off, centered projection
and the working label fixes remained enabled. One hero position was logged in
each capture; view/gaze and ambient animation are not perfectly repeatable.

| Mode | Mean pair cycle | p95 cycle | Mean GPU scene span, both eyes |
| --- | ---: | ---: | ---: |
| Gaze VRS | 22.566 ms | 26.855 ms | 16.042 ms |
| Full shading | 23.240 ms | 29.730 ms | 16.309 ms |
| Gaze VRS repeat | 22.619 ms | 27.026 ms | 16.340 ms |

Mean cycle was ~2.8% lower with gaze VRS, a small preliminary result. GPU scene
spans include CPU queue starvation and did not show a consistent isolated GPU
speedup; CPU time stayed around 14 ms/pair. Do not sell this as a large gain or
as an improvement over fixed foveation, which was not compared here. Artifacts:
ignored `build-frame/gaze-comparison-20261008/{gaze,full,gaze-repeat}.log` and
`summary.json`, sequences 901–903 at ~23:24:10–23:24:38.
The script restored `eye_tracked_foveation=1`, `fixed_foveation=1`,
`profile_capture=0`, `profile_gpu=0` afterward. Game remains running for the owner.
Longer/dungeon tests, tracking-loss behavior and fixed-versus-gaze comparison
remain open; the earlier Frame wake/reconnect crash remains explicitly parked.

Fixed foveation is available: Tier 2 VRS, per-eye optical centers, protected 1x1
ellipse and 2x2 peripheral **opaque** geometry shading. UI, transparency, effects,
shadow-only and unsupported passes stay full quality. Immutable masks are
uploaded once and cached; draw-path lookup uses an epoch to avoid repeated work.
Observed device: Tier 2, 16-pixel tiles; logs confirm reduced-shading draw counts.
User first comparison felt “about the same”; **no measured GPU speedup** claimed.
`[render] fixed_foveation=0` disables; `foveation_inner=0.55` default (0.35–0.85).
Quest 3 does not require gaze for this fixed technique. The eye-tracking source,
tests, settings and validation notes are included in this checkpoint. The
published v0.1.0 preview ZIP still predates these changes.

## Build and verification

Clang 21 with Windows SEH support, MinGW headers/static libraries, CMake 3.28+,
Ninja, OpenXR SDK 1.1.62 and D2RLoader Plugin SDK. See steamvr/README.md for commands.
SDK revisions used:

- D2RLoader PluginSDK: `717f727a0ec52912d1558764345f8fa3453a2bd6`.
- Khronos OpenXR-SDK: `57af7fc61f9f2d492580cb28aab6d0ea59d8d417`.

Run native camera math and deferred-draw tests, `python3 steamvr/session_test.py`,
`python3 steamvr/install_test.py`, shell syntax and diff checks. Windows executable
tests for checked memory reads/writes and buffered logging run through Proton.
The checkpoint verification passed those tests, the 17-call NGX ABI comparison
under both MSVC and MinGW code generation, the profile report on recorded frames,
the cross build and shell/diff checks. The rebuilt DLL with defaults matching
the tested settings has SHA256
`02ed0a87c998cba34303027488672c417ca15746a22be491e07effe8ae2e34c0`.
That build was not installed just to change defaults. The newer gaze pilot
installed later is identified in the foveation section above.
The Windows OpenXR loader is cross-built via `cmake/openxr-loader` with
`cmake/openxr-loader-toolchain.cmake` and packaged with its license notices.
Loader build/export/dependency checks do not establish native Windows gameplay.

## Owner-machine paths and diagnostics

These are local investigation references; friend setup must use its own paths.

- Checkout: `/home/berkybear/diablo2-vr-port`, branch `vr/steam-frame-proton`.
- Steam: `~/.local/share/Steam`, app **2536520**.
- Game: `steamapps/common/Diablo II Resurrected`; prefix:
  `steamapps/compatdata/2536520/pfx`.
- Dependencies: `~/vrdeps`; compiler:
  `~/Android/Sdk/ndk/29.0.14206865/toolchains/llvm/prebuilt/linux-x86_64/bin`.
- Initial complete install/save backup: `build-frame/backups/20261008-171211`.
- Latest pre-deployment snapshot:
  `build-frame/backups/label-order-signature-20261008-230315`.
- Logs: `GAME/d2rloader/logs/d2r-vr-frame.log`, startup `d2r-vr-vrcam.log`,
  `GAME/ReShade.log`, `build-frame/steam-2536520.log` and routing log.
- Local-only profiling: `build-frame/walking-process-samples.csv`,
  `blocking-file-operations.jsonl`, `buffered-logging-process-samples.csv`.
- Game-memory snapshots, saves, account data, logs, builds and captures are
  excluded from git and the release. Do not publish the local game-code dump.

For desktop captures find the current game window with xdotool; IDs/PIDs change.
Frame capture helper in the separate DevilutionX checkout returned a plain blue
image during the latest PCVR session, so do not claim it captured these labels.
Read `/proc` for diagnostic sampling; kernel perf policy blocks normal perf.
Avoid global kernel changes and intrusive tracing while measuring frame times.

## Friend agent's runtime checklist

Read the current WiVRn dashboard setup and compose any required environment
prefix with our Steam wrapper, retaining one `%command%` and the same prefix.
Check a stock OpenXR game first, then our `openxr-probe.exe` / native logs.
The transport requires **XR_KHR_D3D12_enable**, translated by WineOpenXR under
Proton. WiVRn being OpenXR-compatible alone does not prove the game binding works.
Do not switch the owner's active runtime or start another headset stream during
an existing Frame test. Keep Windows launcher and Quest controller mappings
marked untested until verified. No friend invitation is sent without a username.

## Owner headset recording tool

Framecorder 0.1.1 from https://framecorder.coah80.com/ is now installed on the
owner's Frame, using its checksum-verified released ARM64 package. Dashboard,
sync service and automatic update timer are active. The SteamVR dashboard's
Framecorder tab provides Record/Stop; saved videos go to `~/Videos/framecorder`.
Manual recording is configured, game audio on, microphone and instant replay off
while diagnosing D2R performance. Left-stick replay clipping is therefore off.
A four-second headset-source recording was verified: 1920x1080 HEVC, 90 fps,
361 frames and visible passthrough imagery. Temporary test files were removed.

It currently uses SteamVR's headset-view fallback. Direct panel capture needs a
one-time administrator password entered locally on Frame (sudo is not passwordless):
`sudo -v && ~/.local/bin/framecorder-setup --unlock`. Do not request/store the
password in chat or restart the tab during an active recording. The official
installer also exposes the unlock step. No Framecorder source was modified.
