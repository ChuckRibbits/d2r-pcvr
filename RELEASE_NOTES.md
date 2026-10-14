# D2R PCVR v0.3.14 - Windows preview

This update improves physical melee, shield Smite, potion drinking and object
interaction, and changes the defaults for spell aiming. It includes all six
verified PRs #67-#72 since v0.3.13.

Download **D2R-PCVR-Setup-0.3.14.exe** for graphical installation, or
**d2r-pcvr-windows.zip** for portable setup. Existing users should choose
**Update / repair and keep calibration and video settings**, keeping their
existing helper folders and setup backups. Existing saved settings are preserved.

You need your own purchased Diablo II: Resurrected: Steam 3.3.93787 or
Battle.net 3.3.93847 (experimental). Use local offline characters only.
No Blizzard game assets are included; icons and potion models are read locally
from your installed game. SteamVR and Virtual Desktop/VDXR remain supported.
The installer is unsigned; SHA-256 checksums accompany both downloads.

## Physical combat

- **Deliberate melee swings:** slow touches, short twitches and shaft twisting
  no longer count as weapon strikes. Hits require sufficient weapon speed and
  a coherent stroke. Adjust **VR Settings > Combat > Melee hit speed** to suit
  your swing. The default is **12 m/s**, adjustable in 0.25 m/s steps with no
  upper cap. Previously saved values are retained; shield-bash thresholds are
  unchanged.
- **Attack-speed limits:** basic physical weapon hits now respect a cycle
  calculated from character class, weapon class, weapon speed and item Increased
  Attack Speed. A stroke hits one creature, and missed or refused attacks do not
  start the cooldown. Thanks to **@CalMac91** for [#67](https://github.com/bearstonem/d2r-pcvr/pull/67).
- **Shield Smite restored:** physical shield contact keeps its gesture input
  active long enough for the game's skill-bank preparation. Completion still
  cancels it immediately. Native damage and mana rules are retained.
- **Short, sharp hit feedback:** weapon and shield hits use a 10 ms pulse with
  an explicit timed stop, fixing the sustained Steam Frame controller buzz.

## Potions and interaction

- **Potion drinking works with the hidden HUD:** refresh the native belt's
  player ownership before issuing its normal potion-use command. The game's
  own effects, consumption and refusal rules remain in control.
- **More forgiving sipping:** allow more headset clearance and small hand
  movements during the drinking gesture. Confirmation feedback follows the
  hand holding the bottle; one grab still permits only one consumption request.
- **One target per Interact press:** opening a chest no longer also takes its
  dropped loot, and holding Interact over a pile no longer collects everything.
  Release and press again for the next item. Pressing with no target spends
  that press. Thanks to **@CalMac91** for [#70](https://github.com/bearstonem/d2r-pcvr/pull/70).
- **Reliable interaction after menus:** releasing Interact in a menu correctly
  rearms it, so the first fresh press after closing the menu works. Held inputs
  cannot carry an old interaction across menu or game-session changes.

## Spell aiming defaults

New or missing settings now use **Planar spell aiming: Off** and
**Projectile aim assist: Strong**, under **VR Settings > Controls**. Straight
hand spells can aim up and down by default, with launch assistance and gentle
homing toward the selected enemy. Planar aiming, Light assistance and Off remain
available. **Existing saved choices are not changed by an update.**

Charged Bolt stays on the ground and keeps its native wandering behavior, with
launch assistance only. These assist settings do not change bows, thrown
weapons or unsupported native spells, and Strong does not guarantee a hit.

## Validation and remaining limits

The clean native build passed **61/61 checks**. The owner confirmed the combined
changes in an offline Paladin session on Steam Frame / SteamVR with Steam
D2R 3.3.93787. The release DLL was then built from the identical merged source,
installed with settings and saves preserved, and launched into an active
SteamVR/OpenXR session.

All **114 Python checks** passed for setup, repair/removal, launchers, asset
preparation, diagnostics and package staging. The compiled installer lifecycle
fixture passed installation, registration, failed-removal helper retention and
successful removal with an isolated stub backend. These checks do not replace
a new end-to-end installer run against a real game installation.

Attack-speed calculation currently covers basic attacks: skill/aura speed
bonuses such as Frenzy, slows, and dual-wield attack sequences are not modeled.
Unknown class/weapon combinations have no cadence limit. This release has no
new Quest, Index or Battle.net gameplay validation, and does not claim exhaustive
coverage of classes, equipment or long sessions. Battle.net remains experimental.

The dual wheels/casting, hand navigation, VR resolution picker, rendering and
menu fixes from v0.3.13 are retained. The closed experimental performance phase 2
PR (#24) and unrelated local rendering prototypes remain excluded.

See [installation instructions](INSTALL-WINDOWS.md) and [controls](CONTROLS.md).

[Full source comparison](https://github.com/bearstonem/d2r-pcvr/compare/v0.3.13-windows-preview...v0.3.14-windows-preview)
