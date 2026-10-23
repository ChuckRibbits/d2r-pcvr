# Handheld Dynamic Torches: 1H Club Fire Imbution & Spatial Lighting

## 1. Overview & Motivation

In vanilla *Diablo II: Resurrected*, the player's light radius (`STAT_LIGHTRADIUS`) is an omnidirectional 2D/3D radial gradient centered at the character's feet. On a flat monitor, this abstraction is functional.

In Virtual Reality (VR), however, having light radiate passively from your waist or feet breaks immersion. When entering pitch-black dungeons such as the Catacombs, the Kurast Sewers, or the Tombs of Tal Rasha, true VR presence demands that light originates **from a physical torch held in your hand**, dynamically illuminating stone walls, ceilings, and archways in real time as you rotate your wrist and reach into dark corridors.

Furthermore, a torch in a dark fantasy world is not merely an ornament: it is a **one-handed bludgeoning weapon (1H Club)** that can strike enemies, deal physical and fire damage, and ignite combustible elements.

---

## 2. Project Guardrails & Canonical D2 Design

### The "No Save Corruption" Rule
Per `AGENTS.md` and `CONTRIBUTING.md`, the mod strictly mandates:
> *"Preserve upstream credits, licensing, player saves and unmodded desktop play."*
> *"There are no permanent equipment, skill-binding, item-stat or save edits."*

Creating a custom `.txt` item row (such as a fictitious `trc` item code) would serialize foreign data into the player's `.d2s` character save file. Opening that character in unmodded desktop D2R would result in immediate save corruption or load rejections (*"Bad character data"*).

### The Canonical RPG Solution: The Fire-Imbued Club
Instead of fabricating a foreign item, D2R-PCVR qualifies **native one-handed wooden clubs (`clb` - Club, `spc` - Spiked Club)** as functional torches when infused with fire through authentic Diablo II game mechanics:

1. **Socketed Fire (Ruby or Ral Rune):**
   * Inserting a Ruby (Chipped through Perfect) or a Ral rune into a socketed club grants legitimate fire damage. In VR, this ignites the club's head into a live burning torch.
   * *Player Experience:* Early in Act 1, purchasing an open-socket club from Charsi and inserting a found chipped ruby provides an earned, handcrafted torch for the dark descent into the Catacombs.
2. **Unique Club *Felloak* (`clb`):**
   * The canonical level-3 unique club *Felloak* inherently possesses fire damage (+6-8 Fire Damage) and fire resistance. In VR, it serves as the ultimate early-game unique torch.
3. **Magic & Rare Fire Affixes:**
   * Any club rolling fire damage prefixes (*Fiery*, *Flaming*, *Smoking*) or light radius suffixes (*of Light*, *of the Sun*).
4. **The *Enchant* Spell Buff:**
   * When affected by the Sorceress's *Enchant* spell (native state ID 16, skill ID 52) or item charges (e.g., *Demon Limb*), any standard wooden club bursts into intense flame and casts brilliant light for the spell's duration.

---

## 3. Core Architectural Pillars

```
                     +---------------------------------------+
                     |           PLAYER (HMD & Hands)        |
                     |  - Left/Right OpenXR Controller Pose  |
                     |  - Linear / Angular Velocity          |
                     +---------------------------------------+
                                         |
                +------------------------+------------------------+
                |                                                 |
      [Physical Weapon Layer]                           [Spatial Lighting Layer]
   - Evaluates `code[0]` & `code[1]`                  - Evaluates Tip Offset (32cm, +Y)
   - Checks `IsTorchClubCode` ("clb", "spc")          - Multi-harmonic organic flicker
   - Checks Fire Damage / Socket / Enchant            - Flame drag from hand velocity
   - Maps 1H Club swing cadence ("1hs")               - Injects `Light` directly into `g_lights`
                |                                                 |
                +------------------------+------------------------+
                                         |
                       +-----------------------------------+
                       |    D2R Deferred / ReShade Pass    |
                       | - Illuminates ceilings & walls    |
                       | - Feeds `D2R_DepthFog.fx`         |
                       +-----------------------------------+
```

### Pillar 1: Decoupled 3D Spatial Emitter
* OpenXR grip poses define the grip center. The torch head emitter is situated along the tool handle:
  $$\mathbf{P}_{\text{tip}} = \mathbf{P}_{\text{hand}} + \mathbf{R}_{\text{hand}} \cdot \mathbf{Offset}_{\text{shaft}}$$
  where $\mathbf{Offset}_{\text{shaft}} = (0.0\text{ m}, 0.32\text{ m}, -0.06\text{ m})$ extends up through the fist knuckles and angles slightly forward.

### Pillar 2: Dynamic Flame Inertia & Drag
* Real fire is buoyant and floats upward, but air resistance pulls it backward during swings.
* The apparent flame vector is:
  $$\mathbf{V}_{\text{flame}} = \text{Normalize}\left((0, 1, 0) - \mathbf{v}_{\text{hand}} \cdot k_{\text{drag}}\right)$$
  ensuring the flame trails behind rapid weapon swings before rising upright when still.

### Pillar 3: Multi-Harmonic Organic Sinusoidal Flicker
* Static light radii look sterile in VR. Natural flame turbulence is simulated by summing non-harmonic prime-frequency sine waves:
  $$W(t) = 0.45\sin(2\pi \cdot 3.1 t) + 0.30\sin(2\pi \cdot 7.3 t) + 0.15\sin(2\pi \cdot 13.7 t) + 0.10\sin(2\pi \cdot 23.9 t)$$
* This wave $W(t) \in [-1.0, 1.0]$ continuously modulates:
  * **Radius:** $R(t) = R_{\text{base}} \cdot (1.0 + 0.06 \cdot W(t))$ (default $18.0 \pm 1.1$ world units).
  * **Color Temperature:** shifts dynamically between saturated golden amber and hot fire-orange.

### Pillar 4: Engine Render Pipeline & ReShade Injection (Phase 3)
* In `vr/vrcam.cpp` and `reshade/D2R_DepthFog.fx`, dynamic lighting for cave ceilings, rock walls, and the void boundary is driven via uniform arrays `GameLightPos[16]` and `GameLightCol[16]`.
* When a handheld torch is equipped and qualified:
  * **Slot 0 Allocation:** Slot 0 is reserved exclusively for the player's primary active torch. If dual wielding torches, slot 1 is allocated for the secondary torch.
  * **Shader Coordinates:** The torch tip position $(X, Y, Z)$ is transformed relative to the hero and floor: $(X - 	ext{heroX}, Y - 	ext{floorY}, Z - 	ext{heroZ})$.
  * **Radius Scaling:** Uses `ComputeShaderLightRadius(curRadius, ceilRadius)` to normalize against the shader's baseline `CeilTorchRadius` (typically 20.0 units with standard torch reach 19.0), ensuring mathematically exact lighting reach.
  * **Zero Distance Falloff:** The player's handheld torch is assigned intensity $1.0$ (`col[3] = 1.0f`), guaranteeing it never fades or culls based on distance from the character.
  * **Ceiling & Light Activation:** `SetBool(rt, "CeilTorches", ...)` and `SetBool(rt, "GameLightsOn", ...)` are enabled automatically whenever a torch is active, illuminating pitch-black caverns even when no static world torches exist.
  * **Slot Packing:** Remaining slots (1 to 15) are packed seamlessly with the nearest static objects and monster lights from `worldobj::Lights`.
* **Configuration (`d2r_pcvr.ini`):**
  * `[torch] enabled=1`: Master toggle for VR handheld torch lighting.
  * `[torch] radius=18.0`: Base reach in game world units.
  * `[torch] flicker=1`: Enables organic multi-frequency sinusoidal flame flicker.
  * `[torch] warmth=0.65`: Color temperature adjustment blending base amber toward fire orange.

---

## 4. Edge Cases & Robustness

| Edge Case | Failure Mode | Architectural Solution |
| :--- | :--- | :--- |
| **Weapon Swap ('W' key)** | Player swaps to alternate set (e.g., bows). Light could detach or float. | `gamestate::WeaponSet()` is tracked on every tick; if the active weapon set lacks a qualified club, the emitter immediately deactivates. |
| **Off-Hand / Dual Wield** | Barbarian dual-wields or character holds club in off-hand. | `TorchEmitter.isOffHand` is set dynamically based on whether the qualified club resides in `code[0]` (main hand) or `code[1]` (off hand), using the corresponding OpenXR hand pose. |
| **Enchant Buff Expiration** | Club was ignited solely via Sorceress *Enchant*. | The active state status `Has(16)` is continuously polled; upon expiration, the flame and light decay to zero over 0.5 s. |
| **Two-Handed Weapons** | Two-handed mauls (`whm`) or staves (`stf`) with fire damage. | Excluded by `IsTorchClubCode`; only light, one-handed clubs (`clb`, `spc`) qualify as hand torches. |
| **Menu / Loading Transitions** | Inventory open or Act transition screen active. | Checked via `gamestate::MenuOpen()` and `!g_inWorld.load()`; light smoothly dims to prevent visual clutter in 2D UI panels. |

---

## 5. Development Scenario & Testing

* The developer scenario system ([steamvr/dev_scenarios.h](file:///c:/Users/Player%201/Desktop/d2r-pcvr/steamvr/dev_scenarios.h)) provides the **"Torch kit"** option (Choice 41) on the **EQUIPMENT** page.
* In isolated testing (`pcvr-dev`), this supplies a level-1 club (`clb`) ready for immediate socketing or melee testing in Blood Moor and the Catacombs.
* Verified with 132/132 passing CTest test cases (`torch-light-test`, `dev-scenarios-test`, etc.).

