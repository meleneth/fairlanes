# Parties and recruitment

Primary authority for roster structure, recruitment milestones, and recruitment
event operation. Character identity remains governed by
[characters](characters.md).

## Party structure

**Decided:** The intended organization contains five parties, each supporting
up to five characters. Recruitment expands roster capacity; it does not assign
predetermined character classes or innate role specializations.

**Decided:** Each party owns independent progress for every biome tower.
Characters can move among unlocked parties in their account, but tower history
never moves with them. A veteran joining a less-progressed party must use that
party's unlocked biomes and floors.

**Decided:** Transfers are account-authorized requests that commit only when
both affected parties are safely between encounters. While waiting, a party
that reaches its boundary does not begin another encounter. The committed move
must preserve character identity and persistent state, maintain at-most-one
party assignment, enforce capacity, and emit a committed fact. Cross-account
transfers are rejected.

## Origin milestones

**Decided:** Each of the five Origin biome towers grants one recruitment
opportunity at each applicable floor:

| Biome floor | Intended progression |
| --- | --- |
| 5 | Party 1 recruitment |
| 25 | Party 2 recruitment |
| 50 | Party 3 recruitment |
| 75 | Party 4 recruitment |

These are tower-floor milestones, not character-level milestones.

**Decided:** When Party 4 recruits its fifth member, Party 4 becomes complete,
Party 5 unlocks, and five Party 5 recruitment opportunities become available
simultaneously. This is a special completion milestone rather than another
biome-floor tier.

## Story recruits and capacity

**Decided:** Alderion and Bramblethorn remain story recruits and both occupy
Party 1 slots within its five-character capacity.

**Decided:** All five floor-5 opportunities continue to exist. Only three can be
claimed while Alderion and Bramblethorn occupy two Party 1 slots. Unused
opportunities remain assigned to Party 1 and may be claimed if a transfer later
creates a vacancy. They are neither discarded nor redirected, and roster
capacity does not increase.

The opening story route is documented in
[objectives and dialogue](../docs/objectives-and-dialogue.md). Current milestone
and roster behavior is documented in
[Origin recruitment progression](../docs/recruitment-progression.md).

## Event operation

**Decided:** Recruitment and party progression are event-operated:

1. Authoritative gameplay systems commit and emit progression facts.
2. Recruitment systems react to those facts idempotently.
3. The party/account owner performs authoritative roster mutation.
4. The owner emits committed recruitment and roster facts.
5. Story and UI systems observe or request; they do not own roster state.

Repeated milestones must not duplicate opportunities or heroes. Recruitment
must not poll presentation state, and this design does not call for another
event bus.

**Implemented:** The current floor-clear, account-owned opportunity, Village
request, roster mutation, and committed-event chain follows this architecture.
Account-owned character records and immediate/encounter-boundary same-account
transfers are also **Implemented**.
