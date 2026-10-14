# Hand-origin projectile spells

Experimental support for Fire Bolt, Fireball, Ice Bolt, Ice Blast and Charged Bolt on the
Sorceress, using the existing per-hand skill wheels. Assign either hand's
trigger or face-button shortcut. Hold it to repeat; both hands can cast their
own assigned projectile at the same time, with separate aim and cast timing.
Walking and turning remain available during these casts.

Straight free-hand shots start 5 cm along that controller's aim ray. The right casting
weapon uses its visible shaft direction and an origin 12 cm beyond its hand
attachment when a fresh weapon pose is available. This is a hand muzzle,
not an automatically measured staff tip. Each accepted launch freezes its
position and horizontal direction. Pitch is clamped after the weapon pose is
applied, retaining continuous 360-degree yaw and the muzzle's launch height.
Within five degrees of vertical, that hand keeps its last stable heading;
without one, it waits for a usable aim. Turning afterward does not steer a shot.

In **VR Settings > Controls**, **Planar spell aiming** defaults to **On**, including
existing profiles. Turn it **Off** and press **Apply** for full up/down aiming of
Fire Bolt, Fireball, Ice Bolt and Ice Blast. The change applies to new casts from
both hands without restarting; projectiles already in flight keep their original
direction. Charged Bolt retains its native planar wandering in either mode.
The saved preference is `[gameplay] planar_spell_aim=1` (or `0` for free aim).

The native skill-start and skill-dispatch routines validate and consume mana
on the game thread. Each hand supplies its own skill in a scoped selection;
the original selection is restored before returning. No controller cast button
or native casting animation is sent for these supported spells. Each hand
uses the ordinary Sorceress casting frames, including Faster Cast Rate. Two
hands intentionally permit twice the single-hand throughput, at the combined
native mana cost. There is no unlimited or free casting mode.

An insufficient-mana attempt requests the character's normal localized spoken
warning. The server's own mana predicate identifies the failure for that hand's
skill; other native refusals do not produce a false mana warning. Both hands
share a coalesced notification, delivered to the current local client player
on the UI thread. The game's speech routine retains its voice selection and
75-tick repeat cooldown. Notifications expire after 500 ms and are discarded
on a player/session mismatch or while a menu or death animation is active.
Speech does not send a cast input, consume mana or change movement/cast timing.
These optional native entry points are separately checked; an unsupported
speech layout leaves hand casting available without this warning.

Straight projectile flight shares the bow's swept body collision and conservative wall checks.
Before native hit handling, the missile's actual position and room membership
move to the physical contact. This lets native Fireball splash, resistances
and secondary effects use the impact's grid position. Native client hit effects
run separately on the UI thread; they do not apply damage again. The
supported spell VFX retain their game assets, with their authored ground-launch
offsets canceled so straight projectiles start at the hand and Charged Bolt
appears on the launch floor.

Charged Bolt invokes the native volley once per cast. The game evaluates its
bolt count and retains each bolt's wandering path, lifetime, grid collision and
damage. A scoped target query and copied creation request supply the casting
hand's horizontal origin and aim without changing player movement or selection.
The visuals follow each native path's displacement on the launch floor, retaining
the hand's horizontal placement without its height. Charged Bolt still hits
through the game's ground grid. Space for the complete
24-bolt maximum is reserved before dispatch, and mana/cadence apply once per
volley. Each bolt has a separate visual identity. No straight-shot replacement
or second damage calculation runs for these bolts.

Menus, wheels, equipment/assignment changes, tracking loss, world transitions
and released buttons cancel pending requests. A hand's latest pose replaces
older pending samples; there is no accumulated burst after a stall. A cast
already in flight keeps its release direction until impact or native expiration.

## Current limits

- Other classes, charged item skills, other fans, homing projectiles, channels,
  ground effects and utility skills retain native input. This includes Town
  Portal, Lightning and Nova. A native action and physical spells do not start simultaneously;
  a competing button must be released before retrying. Native fallback skills
  can still impose their original movement lock.
- Terrain handling currently shares the bow's launch-floor approximation.
  Walls extend vertically; shots cannot bypass walls by aiming above them.
- Native splash damage still follows Diablo II's grid rules. Secondary impact
  VFX positioning, particle trails, room crossings and weapon muzzle placement
  require headset validation. This is not a claim of headset-tested accuracy.
- Native adapters are signature-checked for D2R 3.3.93787. Unsupported layouts
  leave the original casting path active. Desktop input is unchanged.

## Verification and offline headset checklist

Run `powershell -NoProfile -ExecutionPolicy Bypass -File steamvr/build_windows.ps1 -Test`.
The spell fixtures cover independent button ownership, repeated casts, release
and lifecycle cancellation, FCR boundaries, tick wraparound, planar aim across
all headings, vertical fallback, Charged Bolt eligibility and native path
displacement, and visible emitter alignment. Existing bow, wheel and
controller fixtures must also pass. These fixtures do not execute the injected
game routines.
The feedback fixture also checks simultaneous failures, held-input coalescing,
expiry, menu/death cancellation, character/session changes and clock regression.

On an offline Sorceress in Steam or Battle.net D2R 3.3.93787:

1. Assign Fireball to the right trigger and Charged Bolt to the left. Fire each
   separately, then together at different targets. Check mana deductions and
   the separate repeat intervals, including insufficient mana for both.
   Exhaust mana and try each hand, then hold both: confirm the character's
   mana voice plays without overlap or rapid repeats. Drink a mana potion and
   confirm casting resumes immediately. With sufficient mana, a town refusal
   must not speak the mana warning. Repeat while walking and with each supported
   projectile spell. Audible behavior requires a live headset test.
2. Walk forward, backward and sideways while casting. Physically rotate and
   snap-turn through all headings; tilt the controllers up and down. Check that
   straight projectiles stay level at the muzzle height, without snapping to
   cardinal headings; Charged Bolt stays on the floor even with a raised hand.
   Old shots keep their trajectory.
   Turn Planar spell aiming Off, Apply, and check up/down fire from both hands;
   restore On and confirm new shots return to level flight. Reopen settings and
   restart to check that the chosen mode persists.
3. Check Fireball splash and Charged Bolt's native spread against clustered
   enemies and walls, including a room boundary. Check bolt counts, repeated
   two-handed volleys, hit effects and travel audio in both eyes.
4. Release, reassign, swap weapons, open a wheel/menu, lose controller tracking
   and change areas while holding a trigger. Confirm there are no delayed casts.
5. Check Town Portal and unsupported skills through their existing native path;
   check bow and javelin flight and normal unmodded desktop play separately.

Native behavior was investigated using the locally owned game's code and data,
with the public [D2MOO research](https://github.com/ThePhrozenKeep/D2MOO) as a
reference. No D2MOO implementation or Blizzard game assets are included.
