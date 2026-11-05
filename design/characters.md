# Characters

Primary authority for player-character identity and the sources of combat role.
See [progression](progression.md) for levels and XP and [combat](combat.md) for
how those sources interact in encounters.

## Shared foundation

**Decided:** All player characters share the same fundamental character model.
They have no predetermined classes, innate combat specializations, or intrinsic
combat identities derived from their recruitment biome. Names and recruitment
histories do not independently determine combat power.

Characters with equivalent levels, equipment, skills, and relevant state have
equivalent combat capabilities.

**Planned:** Existing and future content must preserve this classless foundation.
Innate character distinctions may be considered later, but they are not part of
the accepted design. Character classes, innate specializations, and
biome-specific hero abilities require a new explicit design decision.

## Emergent roles

**Decided:** A character's combat role emerges from:

- character level;
- armor type;
- equipped gear;
- learned skills;
- equipped skills; and
- combat strategy.

Armor type is intended to be a primary influence on role. Its particular
bonuses and tradeoffs are **Unresolved**; candidate identities belong in
[`brainstorms/armor-identities.md`](../brainstorms/armor-identities.md).

## Skills

**Decided:** Characters acquire skills through observation and the existing
learning model. Learned/available skills and equipped skills are separate. A
character's ability to learn a skill does not equip it or guarantee that combat
strategy will select it.

**Implemented:** The current grimoire, observation, learned-skill retention, and
equipped-skill paths express this separation. Exact current behavior is covered
by [the Origin Woodland slice](../docs/origin-woodland-slice.md) and the
[combat regression harness](../docs/combat-regression.md).
