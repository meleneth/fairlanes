# Party progression and character development

Canonical intent lives in [`design/`](../design/README.md). This document
describes the current implementation of that intent.

## Ownership and transfers

`AccountData` owns stable `MemberData` character records. Each `PartyData`
holds a `PartyRoster` view of references to the account-owned records. The ECS
`PartyMember` component stores the current assignment and retains the same
entity, closet, equipment, XP, level, grimoire, and learned-skill history when
the assignment changes.

`AccountData::request_roster_transfer(...)` validates same-account ownership,
destination unlock/capacity, and duplicate requests. An idle-to-idle transfer
commits immediately. Otherwise both parties stop constructing encounters after
their current fights and the account commits on `PartyEncounterBoundary` once
both are safe. Request and committed facts use the existing party bus.

## Tower authority

`PartyData` is the tower progression authority. Woodland/Plains unlocks,
highest-cleared floors, frontier floors, selected destination, attempts, and
pending destination are per party.

- A frontier victory emits `TowerFloorCleared` once, advances the frontier, and
  automatically continues.
- A cleared-floor victory leaves the selected farming floor unchanged.
- A wipe retains the selected floor and retries it after town recovery.
- Floor 100 emits `TowerCompleted` once and never constructs floor 101.
- A destination or farming-focus request made in combat is stored and committed
  after encounter cleanup, before another encounter can be built.

An explicit forgotten-skill farming request is a repeat target. If its teacher
floor was also the frontier, the first victory still commits the frontier clear
but the selected destination remains that teacher floor.

## Victory XP

`GrantXPToParty::commit_victory(...)` runs once on normal encounter victory.
It snapshots the encounter's enrolled defenders and awards XP independently to
living members still assigned to that party. Dead participants, other parties,
and later transfers receive none. Raids continue to suppress ordinary XP.

`ExperienceAwardPolicy` is the narrow boundary for future level-relative and
repetition suppression. It currently returns the preserved base encounter
reward without an underlevel bonus. The approved suppression formulas remain
unresolved.

## Forgotten-skill recovery

`Grimoire` retains exact learned-skill history when a normal skill is dropped.
Observe remains in slot zero; the other four slots are normal skills.

Teacher lookup walks actual resolved floor lineups up to the assigned party's
frontier and checks the declared skills of monsters in those encounters. It
chooses the highest accessible floor; ties keep Origin biome order and then
lineup order. The Libram shows whether its selected character knows, forgot, or
never learned a witnessed skill. `f` requests the teacher floor at the next
encounter boundary. Learning still requires the enemy to demonstrate the skill
and the normal Observe/learning/victory path.

Discovery journals are account-owned. Attract mode leaves their listeners
disabled.
