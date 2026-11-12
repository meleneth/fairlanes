# Equipment progression

Primary authority for equipment power, specialization, provenance, and its
relationship to character builds. Character identity is owned by
[characters](characters.md), skill behavior by [combat](combat.md), and the
world structure that supplies equipment by
[cycles and biomes](cycles-and-biomes.md).

## Character builds

**Decided:** Equipment provides meaningful combat-stat bonuses. Character level
establishes baseline power while equipment shapes specialization. A character's
role must continue to emerge from equipment, skills, and strategy rather than
from a fixed class, innate specialization, or biome-specific character ability.

**Decided:** Skills may scale with one stat or several stats. Different stats
may affect different dimensions of a skill, including damage, mitigation,
duration, frequency, accuracy, and healing. A stat is valuable because of the
skills and strategy using it; Attack is not a universal skill multiplier.

**Decided:** Skills should remain useful throughout character progression
without necessarily requiring independent skill-level grinding. The exact
mixture of baseline effects, rank scaling, character level, and equipment-stat
scaling is skill-specific and may be balanced independently.

## Item identity

**Decided:** Tier, slot, biome, and skills are distinct concerns:

- **Tier** determines an item's approximate power budget.
- **Slot** determines which kinds of bonuses an item can provide.
- **Biome** influences the probability distribution of those bonuses.
- **Skills** determine the relative value of those bonuses to a build.

Two items with the same slot and tier may have different stat distributions
because they came from different biomes. Biome influence is probabilistic, not
an absolute restriction.

**Decided:** Higher-tier equipment should generally have a larger stat budget,
but it must not automatically dominate every build. Earlier-biome equipment
must remain potentially useful later, especially when a newly observed skill
changes a character's preferred stats. Fairlanes has no mandatory biome sets or
set bonuses.

### Tier requirements and budgets

**Decided:** Every equipment tier has a minimum character level required to
equip it. Eligibility uses the individual character's level, never party level,
current biome, or highest-cleared floor. Ineligible items remain in inventory
for future use. Manual equipment changes and automatic gearing enforce the same
eligibility rule, and eligible inventory is reconsidered through the existing
level-progression event machinery when a character gains a level.

**Decided:** Every tier defines an inclusive minimum and maximum stat-point
budget. Item generation deterministically rolls within that range, then
allocates the rolled budget among stats eligible for its slot and equipment
type using biome-influenced weights. Tier supplies a budget rather than a
uniform bonus to every stat. An item's rolled allocation is persistent item
data and is not rerolled when effective combat stats are queried.

**Decided:** Ordinary loot and upgraded equipment use one common stat-budget
and allocation policy. Exact level thresholds, budget ranges, stat costs, and
allocation weights are provisional balance choices.

**Decided:** Ordinary equipment retains biome provenance. An operation that
combines items from different biomes must resolve provenance explicitly rather
than silently assigning an arbitrary biome. Unique items retain their declared
identity and explicit modifiers.

## Origin slice

**Decided:** The first equipment-stat progression slice belongs to Origin. The
four cycle identities remain Origin (Physical), Resonance (Magical), Conflict
(Modern), and Singularity (Future). This slice does not establish equipment
mechanics for the later cycles.

**Decided:** Different Origin biomes should favor different stat distributions.
The five biomes and their order are owned by
[cycles and biomes](cycles-and-biomes.md). Particular weights, biome/stat
associations, budget ranges, level thresholds, slot adjustments, stat costs,
and random-allocation formulas are provisional balance choices, not canonical
requirements.

## Armor types

**Unresolved:** Cloth, Leather, and Plate exist, but their mechanical identities
have not been approved. They do not imply character classes, and this document
does not assign identities such as Cloth = magic, Leather = dexterity, or Plate
= strength. Candidate directions and questions live in
[`brainstorms/armor-identities.md`](../brainstorms/armor-identities.md).

The current implementation uses armor kind as an automatic-gearing compatibility
group and historically also applied that group to weapon selection. That is an
implementation assumption, not accepted design.

## Implementation and experiments

**Implemented foundation:** Equipment can already carry Attack, Armor, maximum
HP, and per-channel resistance modifiers. Effective combat stats aggregate the
currently equipped items without permanently copying their bonuses onto the
character. Thump reads effective Attack, and damage resolution reads effective
Armor and resistances. Unique content can declare explicit modifiers.

**Planned first slice:** Ordinary Origin drops receive deterministic,
biome-weighted modifiers; three-item upgrades preserve a coherent
specialization and provenance; automatic gearing compares a small combat-value
score rather than tier alone; and existing inventory/detail views expose item
modifiers and effective character stats.

The following remain **Unresolved** design or experimental balance work:

- the long-term stat vocabulary and any distinction between primary and derived
  stats beyond the existing combat-stat vocabulary;
- exact tier budget ranges and level thresholds, slot adjustments, stat costs,
  biome weights, and rounding/allocation rules;
- which additional skills scale from which stats and at what coefficients;
- how mixed-biome upgrades should work, if they are allowed at all;
- how automatic gearing should understand a character's equipped skills and
  strategy beyond avoiding obvious total-value downgrades;
- final armor-type identities; and
- later-cycle equipment mechanics.

Current numerical experiments and candidate distributions belong in
[`brainstorms/equipment-balance.md`](../brainstorms/equipment-balance.md), not in
canonical requirements.
