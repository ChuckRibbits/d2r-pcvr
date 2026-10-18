# Physical shield blocking

In Unleashed, a raised physical guard can block while standing, walking or
running. Classic keeps the native block rules. The native block/dodge function
still runs, and its eligibility checks remain in place.

## Coverage

Raise the shield and place its broad face toward the attacker. Coverage follows
the final posed shield attachment, independently of headset yaw. Looking away
while keeping the shield in place does not turn its blocking direction. Both
faces can block; an edge-on or horizontally flattened shield cannot intercept
a horizontal attack. Lowering the hand opens the guard.

The incoming native check supplies the attacker position, not an exact 3D hit
point. Coverage is therefore a forgiving horizontal approximation: a 60-degree
surface-angle limit and a 45 cm allowance around the attacker-to-body line.
The attachment may sit up to 20 cm behind the body line to accommodate a guard
held against the chest. These are shared proxy limits, not measured shield mesh
dimensions. Small and large shields do not yet have individually sized collision
surfaces. Item-specific attachment overrides and animated shield meshes can
also differ slightly from the hero attachment used by this approximation.

The hand-height limits remain relative to the headset. Horizontal hand reach
is radial, allowing guards to the side without a gaze-dependent hand box.
Surface position, normal, eye position, scale and hand input come from the same
completed skeleton pose, after body calibration and wrist posing. The normal
comes from the attachment XY plane; its sign does not matter for two-sided
blocking. Source timestamps expire old poses instead of refreshing them when
the UI polls again. Snap-turn epochs invalidate coverage until the new pose
arrives; tracking loss opens immediately. Existing calibration is not changed.

Parries retain their diagonal motion and timing requirements. Guard strain only
changes the visor effect; it does not turn blocking off. Shield bashes have a
separate motion/skill path.

## Investigation and verification limits

The original owner session recorded 13 blocks, 34 open outcomes and one native
check that did not reach the block roll. Eleven otherwise valid guard decisions
were explicitly changed to open by the walking/running restriction, now removed.

In the follow-up headset test the owner reported much better blocking with both
shields, but more apparent misses with the large shield. All 22 rejected guard
decisions were outside the old headset-facing cone, with the hand raised and a
current pose. The large-shield section also had nine native checks which never
reached the block roll. These are not proven landed hits: native flags, block
eligibility or other handling can skip that roll. The new surface coverage has
not yet been verified in a headset. Calibration was not demonstrated to cause
the remaining failures in either captured session.

## Diagnostics and test route

- `Frame guard rejected:` counts decisions rejected by hand height/reach,
  tracking, invalid/stale geometry, `shield_angle` or `shield_position`.
- `[debug] guard_trace=1` adds per-call `Frame guard trace:` and
  `Frame guard native:` plus periodic `Frame guard pose:`. It defaults on in a
  developer build; an explicit saved value still wins. Regular builds default
  it off.
- Traces include surface center/normal, eye and attacker positions relative to
  the player, calibrated units per metre, movement mode and source-pose age.
- Native traces include block/avoid arguments, return value, roll-served status
  and shield durability. A pre-call BLOCK verdict is not a confirmed native
  block when the native function skips its roll.

Use an offline Paladin with each shield. Hold it toward one attacker while
stationary, walking and running; look aside without moving the shield. Move it
to intercept a side attacker, then turn it edge-on, lower it and hold it away
from the attack line to verify those open the guard. Repeat after recentering
and body calibration and while leaning/crouching. Preserve saved calibration
when installing. Verify Classic and shield bashes separately. Synthetic tests
cover transforms, gaze independence, source-pose expiry, native dispatch and
parries; they do not replace headset verification of the visual alignment.
