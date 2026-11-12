# Characters

Primary authority for player-character identity and the sources of combat role.
See [progression](progression.md) for levels and XP and [combat](combat.md) for
how those sources interact in encounters. Equipment specialization and armor's
unresolved mechanical identity are owned by [equipment](equipment.md).

## Shared foundation

**Decided:** All player characters share the same fundamental character model.
They have no predetermined classes, innate combat specializations, or intrinsic
combat identities derived from their recruitment biome. Names and recruitment
histories do not independently determine combat power.

Characters with equivalent levels, equipment, skills, and relevant state have
equivalent combat capabilities.

**Decided:** Every character belongs permanently to exactly one account and
never transfers between accounts. The account owns persistent identity and
development. Party assignment is mutable: a character may belong to at most one
unlocked party in that account and retains level, XP, equipment, learned and
equipped skills, and other persistent state when reassigned.

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

Equipment and skills shape specialization without creating fixed classes.
Armor type's particular bonuses, tradeoffs, and degree of influence are
**Unresolved**; see [equipment](equipment.md) for the authoritative boundary.

## Skills

**Decided:** Characters acquire skills through observation and the existing
learning model. Learned/available skills and equipped skills are separate. A
character's ability to learn a skill does not equip it or guarantee that combat
strategy will select it.

**Decided:** Encountered monsters and witnessed skill descriptions are
discovered per account. Character knowledge and forgotten-skill history remain
per character.

**Decided:** A character can equip four normal skills plus Observe in its
dedicated slot. Dropping a learned normal skill makes it forgotten rather than
erasing the fact that the character once learned it. Recovery requires finding
an accessible enemy teacher and learning through normal observation again; it
does not silently restore or equip the skill.

**Implemented:** The current grimoire, observation, learned-skill retention, and
equipped-skill paths express this separation. Exact current behavior is covered
by [the Origin Woodland slice](../docs/origin-woodland-slice.md) and the
[combat regression harness](../docs/combat-regression.md).

Account-owned character records, mutable same-account party assignment,
account-scoped discovery, forgotten-skill history, and the dedicated Observe
slot are **Implemented**. See
[party progression](../docs/party-progression.md).
