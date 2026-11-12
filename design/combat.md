# Combat principles

Primary authority for established combat-design principles. It intentionally
does not duplicate every skill's current numbers or execution details.

## Sources of combat capability

**Decided:** Character level establishes baseline power. Armor type influences
role. Equipment modifies combat capabilities. Skills determine how those
capabilities are exercised, and characters acquire skills through observation.
See [characters](characters.md) and [progression](progression.md) for the primary
rules.

**Decided:** Skills may value one or several equipment-provided stats, and
different stats may affect different skill dimensions. Scaling relationships
are explicit and skill-specific: Attack is not automatically applied to every
skill, and a universal damage multiplier is not a substitute for meaningful
build choices. Equipment owns the complete rule in
[equipment](equipment.md).

**Decided:** Players and monsters use the same affinity-driven skill evaluation.
Players contribute one active affinity; monsters may contribute a declared set.
A match influences effectiveness but does not universally prohibit off-affinity
use. A multi-affinity monster receives at most one matching contribution for a
single skill.

**Decided:** Declarative scaling may affect multiple stats and effect dimensions,
including damage, mitigation, accuracy, control duration or magnitude, timing,
and healing. Non-numeric skills explicitly declare a neutral policy. Exact
coefficients and caps are experimental implementation data.

**Decided:** Defensive mechanics have domains of effectiveness. No single
defensive mechanism must answer every threat. Significant encounters should
permit more than one viable solution through preparation, party construction,
loadout, and tactical choices.

**Decided:** Party composition, farming destination, biome destination, and
skill-recovery farming are strategic state. Requests made during an active
encounter take effect only at its boundary; combat membership and the current
encounter are never mutated in place.

## Existing defensive lessons

- **Implemented:** Brace mitigates qualifying direct physical damage. Its
  teaching role is to demonstrate timed mitigation with a deliberately limited
  threat domain.
- **Implemented:** Cover redirects qualifying projectiles. Its teaching role is
  target protection and the distinction between aimed and resolved targets.
- **Implemented:** Honey Badger's Eviscerate and Dire Bleed bypass the ordinary
  physical-mitigation answer. Their intended role is to expose the weakness of
  relying only on direct physical defense.
- **Planned:** Origin progression provides cleansing or another effective
  response to that bleed gate. The exact response and acquisition route are
  **Unresolved**.

Exact current effects, timing, and regression observations live in the
[Origin Woodland progression slice](../docs/origin-woodland-slice.md) and
[combat regression harness](../docs/combat-regression.md). Those implementation
documents do not establish future balance requirements.
