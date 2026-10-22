# VR Companion AI: True Ally Behaviors and Spatial Combat Schedulers

## 1. Overview & Motivation

In flat *Diablo II: Resurrected*, mercenaries (hirelings) serve primarily as aura providers and stat sticks trailing automatically behind the player. On a flat 2D monitor, their simplistic pathfinding and rigid state machine are tolerable.

In Realidad Virtual (VR), however, your mercenary is a **physically present 1:1 human-scale companion** standing beside you in dark corridors and chaotic combat. When an AI ally walks directly into your face, blocks narrow dungeon doorways, ignores an enemy flanking you from your blind spot, or suicides blindly into mob clusters while you have a healing potion ready in your hand, immersion is instantly broken.

Because D2R-PCVR already features **physical 3D potion throwing** (where grabbing a potion from your belt and throwing it at your companion heals them on impact), the AI must evolve to be aware of the player's presence, heading, blind spots, and cooperative actions.

This specification details the architecture for transforming D2R mercenaries into genuine, tactical VR allies.

---

## 2. Core Behavioral Pillars

```
                     +---------------------------------------+
                     |           PLAYER (HMD Pose)           |
                     |  - Yaw: Forward Vector                |
                     |  - Headset FOV: ~110 deg              |
                     |  - Blind Spot: 250 deg (Rear / Flanks)|
                     +---------------------------------------+
                                         |
               +-------------------------+-------------------------+
               |                                                   |
      [Vanguard Arc (+30 deg, 2m)]                        [Rearguard Arc (-140 deg, 4m)]
   Act 2 Guard / Act 5 Barbarian                        Act 1 Rogue / Act 3 Iron Wolf
   - Forms frontline beside player                      - Covers rear & overwatch angles
   - Intercepts incoming melee charges                  - Clear line of fire above shoulder
```

### Pillar 1: Dynamic Formations & VR Proxemics

1. **Personal Space Bubble (1.5-meter Clearance):**
   - Allies must never clip through or crowd within 1.5 meters of the player's head/torso.
   - If the player advances or rotates towards the companion, the companion immediately yields space with a sidestep or slight backpedal rather than standing motionless like an obstacle.

2. **Quadrant Formations by Archetype (relative to Player HMD Yaw):**
   - **Melee Vanguard (Act 2 Desert Guard, Act 5 Barbarian):**
     - Positions ~2.0 to 2.5 meters ahead, angled 30° to the left or right of the player's view vector.
     - Establishes a frontline, intercepting approaching enemies while keeping the player's direct forward center line unobstructed for spells, bow aiming, and melee thrusts.
   - **Ranged Overwatch (Act 1 Rogue Scout, Act 3 Iron Wolf):**
     - Positions ~3.5 to 5.0 meters behind or flanking the player's rear quadrant.
     - Maintains clean lines of sight to shoot past or over the player's cover without stepping into the trajectory of the player's physical weapons or spells.

3. **Doorway and Chokepoint Clearance (Anti-Door-Blocking):**
   - When the player is moving toward a doorway, stairwell, or corridor narrower than 3 sub-tiles:
   - If the companion is positioned inside or immediately adjacent to that threshold, the companion yields right-of-way by tucking against the wall or retreating through the threshold.

---

### Pillar 2: Active Bodyguard Instinct ("Peeling" & Blind-Spot Coverage)

In VR, human players have a fixed field of view (~110°) and cannot view the entire screen at once like in desktop isometric perspective.

1. **Blind-Spot Threat Interception:**
   - The engine evaluates the angular difference between the player's HMD forward vector and nearby hostile monsters.
   - Any hostile monster closing within 8 sub-tiles inside the player's blind spots (the 250° sector behind and to the sides of the player) receives **highest targeting priority** from the companion.
   - The companion charges or fires at flanking enemies before they can strike the player from behind.

2. **Peeling during Player Hit Recovery / Stun:**
   - If the player enters `PLRMODE_GETHIT` (hit recovery), freezing, or stun animations, the companion immediately breaks its current low-priority target to attack the specific enemy responsible for striking the player, providing rapid defensive relief.

---

### Pillar 3: Synergy with Physical Potion-Throwing & Critical Health Retreat

1. **Tactical Retreat at Critical Health (< 25% HP):**
   - When companion health drops below 25%, the companion ceases suicidal melee charges and executes an evasive backstep toward the player's forward arc.
   - The companion repositions 2–3 meters in front of the player, establishing a clear, unobstructed line of sight.

2. **Distress Cue:**
   - Upon entering the critical health state, the companion triggers an audible pain/distress sound, alerting the player that assistance is urgently required.

3. **Receiving Stance:**
   - In this retreat posture, the companion avoids erratic sprinting, giving the player a clean, stable trajectory to pluck a healing or rejuvenation potion from their 3D belt and toss it.

4. **Healing Acknowledgment:**
   - Upon potion collision and health restoration, the companion immediately resumes aggressive combat stance, acknowledging the rescue with a sound cue.

---

### Pillar 4: Engine Limits & State Machine Modernization

The native vanilla hireling AI logic (`AITHINK_Fn061_Hireable` at RVA `0x6FCE39E0`) suffers from hardcoded constants that cripple companion responsiveness:

| Vanilla Constant | Hardcoded Behavior | VR Companion Modernization |
| :--- | :--- | :--- |
| `nDist < 25` | Will not engage any monster beyond 24 sub-tiles. | Scaled dynamically with `enemy_sight_pct` (up to 45–48 sub-tiles), enabling companions to engage ranged snipers at matching distances. |
| `nMaxDistance = 24` | Abandons active combat and flees if player steps back > 24 sub-tiles. | Elastic combat leash: while actively in combat, leash expands to 40 sub-tiles; only breaks combat if player enters a different room or exceeds 40 sub-tiles. |
| `nDistance > 100` | Teleports to owner only when over 100 sub-tiles away. | Aggressive anti-stuck watchdog: if pathfinding makes zero distance progress for > 3.0 seconds against collision, trigger immediate safe teleport to player's rear flank. |
| `nDistance < 4` (Rogue/Mage) | 50% idle for 10 frames if escape fails when cornered. | Active backpedal / dodge roll to 10 sub-tiles followed by instant projectile retaliation. |

---

## 3. Architecture & Code Integration

```
+------------------------------------------------------------------------+
|                              vr/vrcam.cpp                              |
|   Extracts live player position, HMD yaw, and room geometry per frame  |
+------------------------------------------------------------------------+
                                   |
                                   v
+------------------------------------------------------------------------+
|                          vr/companion_ai.cpp                           |
|   - Evaluates Formation Anchor (Vanguard vs Rearguard offsets)         |
|   - Calculates Player Blind-Spot Threat Vectors                        |
|   - Watches companion HP for < 25% retreat & potion reception stance  |
|   - Monitors Stuck Timer (3.0s anti-stuck teleport)                    |
+------------------------------------------------------------------------+
                                   |
                                   v
+------------------------------------------------------------------------+
|               Hook: AITHINK_Fn061_Hireable (0x6FCE39E0)                |
|   - Overrides target candidate selection (Blind-spot peel priority)    |
|   - Expands nDist < 25 check to match enemy_sight_pct                  |
|   - Dispatches D2GAME_PETAI_PetMove_6FCE3EE0 for reposition / unstuck  |
+------------------------------------------------------------------------+
```

### Affected Components:
- [`vr/companion_ai.h`](../vr/companion_ai.h) & [`vr/companion_ai.cpp`](../vr/companion_ai.cpp): State machine interceptor, spatial formation math, and unstuck watcher.
- [`vr/companions.cpp`](../vr/companions.cpp) & [`steamvr/companions.h`](../steamvr/companions.h): Companion health polling and UI synchronization.
- [`steamvr/world_coordinates.h`](../steamvr/world_coordinates.h): Coordinate transforms between player head yaw, room tiles, and world space.
- [`d2r_pcvr.ini`](../d2r_pcvr.ini): Configuration flags under `[gameplay]` (`companion_ai=1`, `companion_peel=1`, `companion_formation=1`).

---

## 4. Safety & Compatibility Guarantees

1. **Offline VR Exclusivity:** All hooks and AI overrides execute strictly when `mode() == 0` (local offline game).
2. **Zero Save Corruption:** Modifies only live runtime memory structures; character saves (`.d2s`) and native item tables remain 100% untouched.
3. **Lossless Vanilla Fallback:** When `companion_ai=0`, desktop mode is active, or the plugin is unloaded, original engine code bytes and state machine branches operate without modification.

---

## 5. Phase 1 Implementation Summary

Phase 1 establishes the core state machine modernisation and safety watchdog:

1. **Dynamic Detection Range Modernization:**
   - Companion detection range scales dynamically with `enemy_sight_pct` via `framevr::companionai::ComputeCompanionDetectRange` (baseline 25, scaling up to 48 sub-tiles to match the engine line-of-sight ceiling).
   - Hireling rows in `MonStats` (`roguehire`, `act2hire`, `act3hire`, `act5hire1`, `act5hire2`) are synchronized at runtime through D2RLoader's `DataTableService` in offline VR, and restored to baseline bytes on desktop mode, 100% default, or plugin shutdown.

2. **Elastic Combat Leash:**
   - Implemented `ComputeCombatLeash(bool inCombat)` and `IsLeashExceeded`: expands leash from 24 sub-tiles when idle to 40 sub-tiles while actively engaged in combat.

3. **Anti-Stuck Watchdog & Safe Rear-Flank Relocation:**
   - `StuckWatchdog` tracks hireling animation mode (`MONMODE_WALK = 2`), displacement (< 0.35 sub-tiles per check), and distance to player (> 3.0 sub-tiles).
   - If blocked pathing persists continuously for >= 3.0 seconds, `CalculateUnstuckPosition` calculates a safe spot behind the player's rear flank (4 sub-tiles behind, 2 sub-tiles to the right of the player's view vector).
   - Repositioning executes transactionally via native `RelocateUnit` (RVA `0x491ca0`) and `RoomPoint` (RVA `0x38dfb0`), updating room membership, coordinates, and view bounds safely.

4. **Configuration & VR Settings Integration:**
   - Integrated into the in-VR Settings menu as the ninth row under the **Combat** tab.
   - Displayed as **"Enhanced VR Mercenary"** with selectable options **"Enabled"** (1, default) and **"Legacy"** (0).
   - Backed by `d2r_pcvr.ini` persistence under `[gameplay] companion_ai`.
   - Comprehensive test suite in [`steamvr/companion_ai_test.cpp`](../steamvr/companion_ai_test.cpp), [`steamvr/vr_settings_test.cpp`](../steamvr/vr_settings_test.cpp), and [`steamvr/vr_settings_store_test.cpp`](../steamvr/vr_settings_store_test.cpp).
   - Verification: 128/128 native CTests passed (100%).

---

## 6. Phase 2 Implementation Summary: Formations, Peripheral Visibility & Smooth Proxemics

Phase 2 establishes spatial intelligence, player personal space boundaries, and eliminates artificial teleportation:

1. **Elimination of Artificial Teleportation:**
   - The anti-stuck watchdog threshold was raised from 3.0 to 25.0 sub-tiles (~10 meters), and the timeout was extended from 3.0 to 8.0 seconds.
   - During normal gameplay (walking, exploring, combat, standing still near the player), the mercenary **never** teleports. The mercenary moves smoothly using native Diablo II walking/running animations.
   - Teleportation via `RelocateUnit` is preserved exclusively as a rare emergency catch-up failsafe when the companion is trapped behind distant closed doors or maze barriers.

2. **Peripheral Front-Flank Visibility (No Teleporting Behind):**
   - In VR (110° FOV), companions placed behind the player's back (140°–180°) become completely invisible and feel disconnected.
   - Formation anchors and emergency catch-up locations now place the ally on the **forward peripheral flank** (+35° to +45°, ~2.0–2.5m ahead-side).
   - This ensures the player can always see their companion out of the corner of their eye, feeling their constant presence, while leaving the central 0° combat/aiming corridor unobstructed.

3. **Archetype Quadrant Formations:**
   - Classified hirelings into `MeleeVanguard` (Act 2 Desert Mercenary, Act 5 Barbarian) and `RangedOverwatch` (Act 1 Rogue Scout, Act 3 Iron Wolf) via `GetHirelingArchetype`.
   - **Vanguard Anchor:** Positions 4.5 sub-tiles (~1.8m) ahead on the right flank (+35°), ready to intercept melee threats.
   - **Overwatch Anchor:** Positions 5.5 sub-tiles (~2.2m) ahead on the flank (+45°), providing support fire while keeping distance from frontline collisions.

4. **Personal Space Bubble & Doorway Clearance:**
   - Implemented `CheckPersonalSpaceViolation` (3.75 sub-tiles / ~1.5m) and `IsDoorwayObstructed`.
   - Native hireling AI moves smoothly around the player without artificial hitching.

---

## 7. Phase 3 Implementation Summary: Survival, Tactical Retreat & Potion Synergy

Phase 3 implements companion self-preservation instinct, audible audio alerts, and seamless coordination with physical VR potion throwing:

1. **Critical Health Tactical Retreat (< 25% HP):**
   - Implemented `TacticalRetreatController` (`CriticalHealthRatio = 0.25f`): when companion life drops below 25%, they cease suicidal offensive engagements and fall back toward the player.
   - Hysteresis recovery (`RecoveryHealthRatio = 0.50f`): the companion remains in defensive retreat until life recovers to at least 50% HP or until a potion is confirmed received, preventing rapid oscillating state transitions.
   - Aggro de-escalation: while in retreat, `EffectiveDetectRange()` clamps to default (25 sub-tiles) so the companion does not acquire new distant targets while heavily wounded.

2. **Audible Distress Alerts:**
   - When entering tactical retreat, the companion immediately triggers an audible sound cue via `bowvr::PlayHeroSound(g_distressSound.load())` dispatched to the UI thread, alerting the player without requiring HUD inspection.
   - Repeats periodic reminders every 8.0 seconds (`DistressCueCooldownMs = 8000`) if the companion remains critically low in danger.
   - Configurable via `[gameplay] companion_distress_sound` in `d2r_pcvr.ini` (default `398` `block_weapon_1`; `0` to mute).

3. **Receiving Stance for Physical Potion Throwing:**
   - Implemented `CalculateReceivingStancePosition`: positions the retreating companion ~2.6m ahead (6.5 sub-tiles) at a gentle +20° forward flank angle.
   - Within the player's primary VR FOV, providing a clean, unobstructed throwing arc to pluck potions from the 3D belt and toss them.
   - Evaluated by `TacticalFormationController` with a 2.0 sub-tile deadband (`ReceivingStanceDeadband`), holding stance stably without erratic sprinting or evasive zig-zagging while the player aims the potion throw.

4. **Healing Acknowledgment & Potion Consumption Synergy:**
   - Hooked `interactionstate::Apply` for `Action::FeedMerc`: when a thrown potion hits the companion's feet and native consumption is confirmed, `companionai::OnPotionReceived()` immediately signals recovery.
   - The companion exits tactical retreat, plays the native healing overlay (`HealOverlay = 26`) and vanilla speech acknowledgement ("Thanks!"), and transitions smoothly back to combat stance.

5. **Adaptive Rescue Watchdog:**
   - When in retreat, `StuckWatchdog` adapts with a more urgent threshold (`RetreatStuckDistanceThreshold = 15.0f` sub-tiles, `RetreatStuckTimeoutMs = 4000ms`), rescuing the companion to the receiving stance in front of the player if trapped behind barriers while critically wounded.
