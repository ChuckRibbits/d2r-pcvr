# Private test feedback follow-up

Feedback was recorded against private build `20261013-6abf4e5`. The changes below
address that build's menu comfort, discovery, map and casting feedback.

## Settings and behavior

| Feedback | Change |
| --- | --- |
| Menus too close | VR Settings > HUD > Menu distance: Close / Medium / Far. Medium is the default, 30 cm farther than the previous build. Close retains the previous placement; Far adds 60 cm. Inventory/merchant anchors are 0.55 / 0.85 / 1.15 m; main menus are 0.70 / 1.00 / 1.30 m. Their size and eye-level alignment remain fixed. NPC speech keeps its separate placement. |
| Controller picker hard to find | The VR tab has a larger highlighted dropdown, an explicit controller-label heading and detected left/right controller names. Automatic and the Valve Index override remain available. |
| Two hand wheels not obvious | Controls help explains clicking either stick and assigning with the opposite hand's laser. Each hand wheel also explains assignment and the top trigger slot's compatible weapon gestures. |
| Compass option hard to find | The HUD tab explicitly labels the combined Minimap & Compass control, including Off and the independently accessible wheel map. |
| Raising a hand opens a full-screen map | While VR requests map capture, borrow a native corner layout and restore the previous transient layout when capture ends. The game's saved map setting is not rewritten. Both hand and wheel capture benefit. |
| Tooltip hover depends on aim position | Use exact laser coordinates for the item grid's draw-time hover query as well as pointer dispatch. Previously, drawing could fall back to the controller-selected cell after laser dispatch ended. |
| Tooltips can obscure other items | HUD > Item tooltip timing offers Instant (default), Short (300 ms), and Original. Short tracks the native item and widget, so movement within a multi-cell item does not restart the timer. Switching items, dragging, losing the ray or a long frame gap resets the dwell. |
| Equipped comparisons unclear | Retain native comparison generation and eligibility, requesting comparison only at the item-tooltip Shift query. No keyboard Shift event is generated for purchasing or transfers. The main tooltip and its comparisons are delayed together in Short mode. |
| Charged Bolt too quiet | Restore the skill's native casting-start sound, which direct casting bypassed along with the character animation. Play once per successful cast, independently per hand, not once per volley member. Keep the game's volume, audio channel, travel sounds and native sound redirection. This also restores start cues for the other supported hand spells. |
| Missing out-of-mana feedback | Separately addressed by PR #57. The original private ZIP predates that fix; the next combined candidate must include it. |

All native changes are scoped to active VR ownership and signature checked for
D2R 3.3.93787. Desktop pointer/controller behavior and saved native options are
preserved. The map layout and audio fixes reuse native functions and installed
game data; no extracted game assets are distributed.

## Verification

- Native Windows build and all 56 checks passed. After adding further settings
  assertions, both updated settings interaction and INI tests also passed.
- Added coverage for menu preset geometry, exact item cursor-query scope,
  per-item dwell and reset conditions, temporary map layout restoration,
  one casting cue per volley/per hand, stale audio rejection, and setting
  defaults, normalization and persistence.
- Inspected synthetic Controls, VR and HUD previews for readable labels and
  non-overlapping controls.
- Compared all four newly used native signature regions with the locally owned
  game's existing code snapshot. Runtime guards check them again before use.
- Index/Quest headset testing and audible validation have not been performed.
  The socket report is tentative: the hover correction addresses one concrete
  cause, but does not establish that every socket-related visual overlap is fixed.

## Offline tester checklist

Use local offline characters only. Record DLL/build identifier, Steam or
Battle.net edition, headset, controllers, connection/runtime and resolution.

1. On Index, check Automatic identifies both controllers and that selecting
   Valve Index shows A/B and Grip + A/B on both wheels. Check each stick click,
   opposite-hand laser, trigger and face-button assignments. Repeat on Quest
   with left X/Y and right A/B. Verify settings survive restart.
2. Turn NVIDIA DSR off if previously enabled. In VR Settings > Video select
   4096 x 4096 and Apply. Confirm the reported source resolution and compare
   clarity/performance with a lower preset. High native graphics settings or
   VD quality alone do not establish the game's actual render resolution.
3. Try all menu distances with inventory, character, merchant and main menus.
   Check eye-level content, full tooltips, readable text and matching laser hits.
   With the sky enabled outdoors, open NPC choices, continue speech and open the
   journal; verify the sky stays visible behind these panels. Repeat with fog
   enabled and, indoors, an enabled cave ceiling. Enter a merchant or waypoint
   and check the shared black background, then return to dialogue and close it.
   Check both eyes during transitions. Headset verification is pending.
4. Start with the game's full-size map setting, then raise the wrist/palm and
   open each wheel's MAP tab. Expect a compact map and no full-screen map flash.
   Lower the hand, open menus and leave/rejoin a character; confirm native map
   visibility/layout restoration. Repeat with left/right corner settings and
   hand navigation Off. Compass and map should appear/hide together.
5. Sweep the laser over every occupied cell of a tall/wide item, its edges and
   its sockets in inventory and a merchant. Check ordinary, empty-socket and
   socket-filled items. Expect a stable item tooltip and eligible equipped
   comparison. Check another item, empty cells, dragging and controller
   navigation. Try Instant, Short and Original; verify purchasing/stack
   shortcuts still honor only the player's actual modifiers.
6. Cast Charged Bolt alone, then alongside Fireball, while stationary and
   moving. Listen for one normal casting cue per hand per accepted cast, with
   no burst proportional to bolt count. Check native effects volume/mute.
   With PR #57 included, drain mana and verify the character warning and its
   native cooldown. Separately check optional low-resource alerts if desired.
7. Test Fire Bolt, Fireball, Ice Bolt, Ice Blast and Charged Bolt on each hand,
   alone and together while walking, physically turning and snap turning.
   Straight projectiles should start at hand/weapon height; Charged Bolt stays
   grounded. Repeat with planar aiming On/Off. Unsupported native skills still
   have their documented native behavior and may lock movement.

If sockets still obscure a tooltip, capture whether the tooltip disappears,
changes to another item, or remains visible behind socket artwork. These are
different failure modes and need a specific reproduction before changing
render order or hiding socket information.
