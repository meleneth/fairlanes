# Character resources and consumables

Primary authority for combat-resource state, consumable action semantics, and
temporary consumable buffs. Character identity remains owned by
[characters](characters.md), skill effects by [combat](combat.md), and durable
gear by [equipment](equipment.md).

## Character resources

**Decided:** Resources belong to characters; resource interactions belong to
skills. Every character can possess every defined resource. Resource identity,
maximum capacity, current amount, recovery or generation, expenditure, scaling,
and effect-driven modification are independent declarations. Resources do not
determine class, affinity, or available skills.

**Decided:** Adrenaline represents offensive momentum. Reflexes represent
defensive readiness. Neither inherently adds Attack, Armor, accuracy, evasion,
or affinity power. A skill must explicitly use, generate, restore, spend, or
scale from a resource for that resource to affect combat. Mana is not part of
this slice.

**Decided:** All resource changes are bounded by zero and capacity,
deterministic, and observable. A resource-independent skill remains eligible at
zero resources. An unaffordable resource-dependent skill is ineligible without
blocking other productive choices.

## Skill action semantics

**Decided:** Resource scaling normally reads a pre-expenditure snapshot. A
fixed cost commits once when the authoritative action begins, after eligibility
and before effect evaluation. An action rejected before it begins spends
nothing. Once begun, interruption does not refund its committed cost unless a
future skill explicitly declares refund semantics. Generation on hit occurs
only for a resolved qualifying hit; restoration on resolution occurs once.

**Implemented evaluation order:** validate eligibility; capture current and
maximum pools; commit the fixed cost; calculate base plus bounded stat and
resource contributions; apply the one matching affinity multiplier; resolve
the gameplay effect; then apply declared on-hit, on-resolution, and target
resource changes once. This order is deterministic and shared by player and
monster actions. Different post-cost semantics remain possible only through a
future explicit declaration.

**Implemented first slice:** Thump generates Adrenaline on a landed hit. Thump
Around spends Adrenaline and scales damage from its pre-cost amount. Venom
Needle scales damage from remaining Reflex percentage. Brace restores Reflexes,
and Web Snare spends Reflexes. Exact capacities, costs, generation, scaling
coefficients, and caps are experimental.

## Potions and flasks

**Decided:** Potions are finite inventory items with an immediate effect. Using
one in combat consumes one ordinary action opportunity and one quantity exactly
once. Initial potions restore HP, Adrenaline, or Reflexes, bounded by the
target's maximum. Inventory ownership, living-target eligibility, encounter
authority, and party boundaries apply; inventory access never supplies free or
unlimited healing.

**Decided:** Flasks apply temporary effective-stat modifiers over the existing
simulated world clock. They do not immediately restore HP or resources. Initial
flasks modify effective Attack, effective Armor, or ATB charge rate. Identical
effects replace and refresh one another instead of stacking; different effect
kinds may coexist. Effects persist across ordinary encounter and town
transitions and expire deterministically while world time advances.

**Decided:** Consumables are party-owned inventory. Character reassignment does
not duplicate or implicitly move party inventory. Active buffs and character
resources remain character state when a character moves between parties.
Persistence through a future save/load format must preserve quantities,
resources, and absolute remaining simulated duration; no save loader currently
exists, so disk persistence remains **Planned** rather than implemented.

## Autonomous policy and inspection

**Decided:** Autonomous potion use is conservative and explainable: do not heal
at full HP, avoid gross overhealing, reserve healing for danger, do not restore
a full resource, and do not retry unavailable inventory. Resource-ineligible
skills are skipped in favor of eligible productive actions. Flask use is an
explicit preparation decision, not an automatic per-encounter habit.

**Implemented first slice:** Character inspection and the Libram expose current
and maximum resources, active flask duration, available consumables, skill
costs/generation, authoritative resource scaling, and ineligibility reasons.
Combat and UI share the same skill definitions and evaluator. Regression
telemetry records starting values, changes, rejection, consumption, healing and
waste, and flask application/expiration.

The exact starting pools, potion magnitudes, flask bonuses and durations,
autonomous thresholds, and skill coefficients are **Unresolved balance
experiments**, not settled design values.

**Decided:** Alchemy recipes produce these same finite potion and flask item
types. Crafting changes acquisition and provenance, never their authoritative
combat effects or duration semantics. See
[professions and production](professions-and-production.md).
