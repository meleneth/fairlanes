# Origin recruitment progression

The Ruby content DSL is authoritative for recruitment identities and triggers.
It declares twenty biome milestones plus five party-five opportunities; generated
`fl/generated/recruitment_content.hpp` is the runtime table and the generated
[recruitment review](tour/generated/recruitment-progression.md) lists every row.

| Committed tower clear | Destination | Opportunities across Origin |
| --- | --- | ---: |
| Floor 5 | Party 1 | 5 |
| Floor 25 | Party 2 | 5 |
| Floor 50 | Party 3 | 5 |
| Floor 75 | Party 4 | 5 |

Each tier has one opportunity for Woodland, Plains, Murky Swamp, Caves, and
Desert. The five tower biomes may reach a tier in any order. Party 2, 3, or 4 is
created when its first opportunity becomes available; filling the preceding
party is not required. Account party storage is sequential, so an artificially
injected higher-tier clear whose destination cannot yet be created remains a
committed pending fact. It resolves when the missing lower party slot becomes
available. Production tower progression naturally reaches tiers in order.

When party 4 commits its fifth member, it emits one roster-completed fact,
unlocks party 5 once, and changes all five generated party-five opportunities
to Available before emitting their unlock events. Party 5 therefore receives a
whole roster of opportunities at once rather than another biome tier.

## Opportunity and roster state

An opportunity is Locked, Available, or Claimed. A committed floor clear is
queued by the account's typed listener and resolved after party advancement on
the same production beat. This prevents nested event dispatch: every observer
sees `TowerFloorCleared` before the derived opportunity event. Committed clear
identities and opportunity states make retry and replay facts idempotent.

Available opportunities persist until successfully claimed. The account owner
validates the opportunity, destination party, five-member capacity, and stable
hero name before calling the party's ordinary member-construction path. Only
after the member exists does it mark the opportunity Claimed and emit
`HeroRecruited`. A full roster then emits `PartyRosterCompleted`; party creation
commits before `PartyUnlocked`. Failed or repeated requests emit no committed
recruitment fact.

The existing Alderion and Bramblethorn story choices are special recruitment
identities outside the twenty biome milestones. They use the same account and
party mutation path and occupy two of party 1's five slots. Consequently only
three of its floor-5 opportunities can be claimed. The other two stay Available
on the recruitment board: they are neither discarded nor converted into extra
members.

Ordinary generated recruits use the existing deterministic hero-name sequence
for their destination party and receive the same opening Thump setup as the two
story recruits. Future content may attach dialogue, skills, or named identities
to the stable opportunity IDs without changing progression state.

## Event ownership

```text
PartyData commits latest-floor progress and emits TowerFloorCleared
  -> AccountData records and resolves the generated milestone
  -> AccountData creates the destination party when needed
  -> AccountData emits PartyUnlocked and RecruitmentOpportunityUnlocked
  -> Village emits RecruitmentClaimRequested on the existing StoryBus
  -> ObjectiveSystem routes the request to AccountData
  -> AccountData validates; PartyData commits the member
  -> AccountData emits HeroRecruited and, once, PartyRosterCompleted
```

The Village is presentation and request routing only. It never constructs a
hero or maintains a second opportunity state. FTXer's generated composition
continues to own navigation.

## Manual play and regression

From a selected party screen, press `v`, choose **Visit the recruitment board**,
and accept an Available row. Rows identify the destination party and the biome
floor (or party-4 completion) that unlocked them. Claimed rows remain visible.

Run `scripts/regress recruitment` for the concise schedule and event check. Its
`[recruitment]` production integration test advances real encounters through
Woodland floor 5, observes the committed floor clear and opportunity in order,
then claims the recruit through the Objective System request path. Focused tests
cover all twenty milestone identities, duplicate facts, retained pending facts,
out-of-order biomes, capacity rejection, exact-once completion, the five-at-once
party-five transition, stable naming, and the manual Village interaction.
