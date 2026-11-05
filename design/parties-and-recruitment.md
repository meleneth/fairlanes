# Parties and recruitment

Primary authority for roster structure, recruitment milestones, and recruitment
event operation. Character identity remains governed by
[characters](characters.md).

## Party structure

**Decided:** The intended organization contains five parties, each supporting
up to five characters. Recruitment expands roster capacity; it does not assign
predetermined character classes or innate role specializations.

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

**Unresolved:** How those two occupied slots interact with the five floor-5
biome opportunities. Do not silently discard excess opportunities, create extra
party slots, or treat current behavior as the design answer. The current
implementation leaves two opportunities available after Party 1 fills; that is
an implementation choice pending explicit approval.

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
