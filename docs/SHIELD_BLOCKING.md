# Physical shield blocking

In Unleashed, a raised physical guard can block while standing, walking or
running. Classic keeps the native block rules. The native block/dodge function
still runs, and its existing eligibility checks remain in place.

## What the guard currently checks

The left controller must be in the chest guard zone, with a recent posed shield.
Attackers must be within 60 degrees of the headset's horizontal facing direction.
The guard does not currently test the visible shield mesh, its dimensions or
its surface orientation. A larger model does not grant a larger guard zone.
Lowering the hand, looking away or losing tracking can therefore open the guard
even when part of a large shield appears to cover an enemy.

Parries retain their diagonal motion and timing requirements. Guard strain only
changes the visor effect; it does not turn blocking off. Shield bashes have a
separate motion/skill path and are not the same as holding a guard.

## Investigation

The owner's captured session recorded 13 blocks, 34 open outcomes and one native
check that did not reach the block roll. Eleven otherwise valid physical guard
decisions were explicitly changed to open by the old walking/running restriction.
The restriction has been removed at the incoming-hit adapter.

Both a buckler and a Paladin shield produced successful blocks. The session did
not record individual rejection reasons, so it cannot establish why the remaining
stationary attempts failed or prove a shield-size-specific defect. No guard-zone
thresholds or shield geometry have been changed based on that incomplete evidence.

## Diagnostics and test route

- `Frame guard rejected:` counts rejected decisions by reason: hand too low,
  high, behind, far, wide or across; invalid/untracked hand; stale shield pose;
  or attacker outside the view-direction cone. These are decisions before the
  native block/dodge function, not damage totals.
- `[debug] guard_trace=1` adds `Frame guard trace:` for each eligible incoming
  hit and `Frame guard pose:` every two seconds. It defaults on in a `-Developer`
  build; an explicit saved value still wins. Regular builds default it off.
- Traces include the verdict/reason, native movement mode, hand position in
  metres from the head, direction alignment and pose/parry timing.

Use an offline Paladin with each shield. First hold it at chest height and face
one attacker while stationary, then repeat while walking and running. Also try
lowering the shield and looking away to confirm those still open the guard.
Compare any missed blocks with the trace before changing the zone or introducing
mesh-based coverage. Verify Classic separately. No headset result is implied by
the synthetic adapter or pure guard tests.
