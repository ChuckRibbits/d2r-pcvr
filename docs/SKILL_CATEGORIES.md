# Class categories in the wheel skill picker

Open either hand's wheel and choose a button slot. The picker starts on **All**,
with the existing native catalog order. Three tabs follow the character's skill
trees, and **General / Items** contains neutral actions, foreign-class skills
and item-owned charges. Tab counts refer to available assignable skills, not
all learned passives. Empty tabs are disabled. Previous/Next pages within the
selected group; switching groups starts at its first page. Clear Slot and Back
keep their existing behavior. There is no settings or save migration.

Supported trees:

- Amazon: Bow & Crossbow; Passive & Magic; Javelin & Spear.
- Sorceress: Fire; Lightning; Cold.
- Necromancer: Curses; Poison & Bone; Summoning.
- Paladin: Combat; Offensive Auras; Defensive Auras.
- Barbarian: Combat; Masteries; Warcries.
- Druid: Summoning; Shape Shifting; Elemental.
- Assassin: Traps; Shadow Disciplines; Martial Arts.
- Warlock: Demon; Eldritch; Chaos.

`skill_categories.h` stores only skill ID/category facts, audited against the
installed 3.3.93787 Skills -> SkillDesc.SkillPage and CharStats tree names. No
game tables or assets are distributed. Unknown IDs/classes remain accessible
through All and General / Items. If a supported game update changes tree
membership or adds skills, revisit these facts; there is no guessed native
table pointer or memory offset for categorization.

The identity-checked native player class is captured independently of developer
mode. Category navigation filters a frozen catalog without changing skill IDs,
item owners, native assignment rules or combat behavior. The assignment request
still revalidates the exact skill/owner, character session, weapon bank, and
previous binding. Paint context includes class, category and counts so a stale
atlas cannot be clicked after filtering. Rendering and hit testing share bounds.

## Verification

Run `steamvr/build_windows.ps1 -Test`. `skill-categories-test` covers all class
trees, general/item fallback, counts, page reset, both hands, exact item-owned
assignments after catalog reordering, held-trigger suppression, stale identity
rejection, cancellation and paint invalidation. Existing interaction tests cover
the legacy eight-slot editor as well. `skill-categories-art-test` checks that all
class headings fit and renders representative picker pages for visual review.

Offline headset checks still required: on Sorceress, Paladin and Necromancer,
open each hand's wheel, filter trees, page through All, assign a skill, clear it,
and cancel without changes. Try an equipment-granted skill and change weapon
sets. Confirm all tabs and the last row remain readable and easy to point at.
