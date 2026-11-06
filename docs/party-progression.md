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

`PartyData` is the tower progression authority. Origin biome unlocks,
highest-cleared floors, frontier floors, selected destination, attempts, and
pending destination are per party.

All five Origin towers now use that authority. Clearing floor 9 of Woodland,
Plains, Murky Swamp, or Caves unlocks the next ordered biome for that party.
Each tower currently has a generated 100-floor fallback pattern so it remains
playable while authored encounter design is refined.

## Origin automatic progression

`OriginAutoProgression` observes the controlled party's committed victory,
wipe, encounter-boundary, floor-clear, biome-unlock, tower-completion, and
membership facts. It also observes account progression-bus recruitment facts
and the story autoplay toggle. Handlers only mark a decision pending; evaluation
runs after the account's production beat and recruitment resolution.

The controller issues the existing `PartyData::select_tower` request and
`RecruitmentClaimRequested` story command. It never constructs an encounter:
the normal `PartyLoop` remains the encounter scheduler in both manual and auto
modes. Auto selects the newest sequentially unlocked Origin biome and claims the
first legal opportunity for its party. Manual mode disables these strategic
decisions without rewriting tower state.

The story autoplay toggle applies to all parties in the account. GrandCentral
owns one controller per party, synchronizes all existing controllers through
the shared toggle event, and creates an enabled controller for a party unlocked
while autoplay is already active. Controllers do not share tower or recovery
state.

After three failed attempts, auto retreats to the highest cleared floor, farms
two victories, and retries the frontier. Three failures while recovering move
the farm target down one floor. If floor 1 cannot be won and no lower legal
recovery exists, the controller blocks new auto encounters until autoplay is
disabled or re-enabled. This bounded policy is deliberately simple and is not
a general strategy engine.

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
