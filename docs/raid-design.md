# Account raids: design requirements

Status: design in progress, recorded 2026-09-06. These requirements describe
planned behavior; account raids are not implemented. Settle the open decisions
before implementing their dependent mechanics. This document is the working
reference for subsequent raid design discussions.

## Confirmed encounter and outcome rules

- A raid is one shared encounter involving all five parties of an account
  (25 characters at the planned five characters per party).
- One party wiping does not end the raid. The remaining characters continue
  fighting the same enemies.
- Victory requires all enemies dead and at least one participating character alive. Defeat requires
  all participating characters to be dead. A boss dying while its adds remain
  alive is not sufficient for victory under this rule.
- Victory is all or nothing for the account. Every participating party receives
  victory credit, including a party whose entire membership died before the win.
- If the last enemies and characters die together, **defeat takes precedence**.
  This is a raid wipe and unlocks a dedicated raid mutual-destruction achievement.
  Ordinary combat mutual destruction does not qualify for that achievement.
  Resolve the shared outcome once after the relevant action/effect resolution;
  do not let callback ordering publish victory before the mutual wipe is known.

## Confirmed first arrival and retry policy

For now, start the first Visitor raid **immediately at day zero**, once the account
roster is initialized. Do not wait for a full Visitor interval or player readiness.
The expected early result is a fast loss with no skills learned; this provides
immediate access for iterating on raid setup and animation. This expectation is
not a special rule disabling skill learning or forcing defeat.

Consume that arrival once. After any raid outcome, **there are no retries**:
the account waits for the next Visitor. The day-zero calendar value must not
relaunch the raid on every beat or when the account calendar resumes.

The intended later startup is day one instead of day zero, which avoids the
immediate conjunction. That is a future starting-calendar change, not the current
behavior to implement. Offline arrival behavior remains an open decision; missed
arrivals must not become banked retries.

## Confirmed arrival condition

Characters enter the automatic Visitor raid with their **current HP and living/dead
state**. No entry healing and no resurrection: injured characters stay injured,
and dead characters stay dead. An already-dead party can still receive shared
victory credit. All characters dead at arrival must resolve consistently with
the collective defeat rule; the exact entry/outcome event sequence needs design.

**Combat statuses do not transfer.** Remove prior-fight buffs, debuffs, associated
visual overrides, listeners, and scheduled status work through explicit cleanup.
Do not migrate their remaining durations or callbacks into the raid. This
supersedes the earlier status-carryover requirement. Cleanup itself must not heal,
revive, or apply an extra status tick while the characters are being transferred.
Persistent character progression, equipment, and retained skills are unaffected.

## Confirmed summoning and skill retention

At raid start, remove every participating player from their current combat and
summon them into the shared raid encounter. They do not continue participating
in their old fights. Preserve current HP and death state; clear old combat statuses as specified above.

**Skills acquired through observation in those interrupted fights are kept.**
Summoning is a legitimate extraction: the characters did not die and were called
to the raid. Timing the Visitor to escape a fight with an observed skill that
would otherwise be lost is an intentional gameplay loophole. Do not "fix" this
by requiring victory in the interrupted fight or rolling the skill back on entry.

This retains successful observation learning already acquired in that fight;
it does not turn every skill merely witnessed for the libram into a learned
character skill or bypass existing Observe rank/chance checks. Settle the old
fight's pending skill retention at summoning, so its wipe/victory subscriptions
cannot later revoke those skills during the raid. Skills learned within the raid
are a separate acquisition scope governed by the raid's eventual outcome.

Summoning must have a specific event-driven exit reason. It is not an enemy
kill, an ordinary-fight victory, a wipe, or a successful Flee roll. Keeping skills
must not require emitting a fake victory event, awarding unearned kill loot/XP,
or inflating statistics and achievements. An interrupted fight and a newly
started raid are separate lifecycle facts with separate identities.

Required handoff behavior, with concrete event names still to be designed:

1. Capture the participating roster and transfer state before old-encounter
   cleanup can erase it.
2. Publish the explicit raid-summoning/exit fact through the event system;
   skill-retention listeners settle successful observations exactly once.
3. Detach participants from the old combat/ATB and cancel old combat actions so
   enemies or delayed attacks cannot continue hitting them from that encounter.
   Clear old combat statuses, visuals, subscriptions, and status callbacks.
   No status timing is transferred, and cleanup must preserve HP/death state.
4. Enroll the participants in the one raid and publish its start fact once.

Make this ordering explicit in named orchestration code and event contracts.
Prevent reentrant/double processing. The handoff must not allow a global beat
between detaching some parties and enrolling others to produce extra attacks.
The exact policy for empty old encounters and characters previously dead remains
part of lifecycle design, not permission to heal or rejoin old combat.

## Raid-exclusive rewards

Start with **two drops per participating party**: a standard five-party raid
therefore provides **ten drops**. The count is based on participation, not the
number of surviving parties. A wiped party still counts when the account wins.

Drops must be special items exclusive to raids. Trinkets are the current example,
not a finalized item category or equipment-slot contract. Ordinary farming and
non-raid loot tables must not award these items.

Working interpretation: the ten-drop allocation is a victory reward. Failure
rewards, ownership/distribution, duplicates, and whether two items are assigned
to each party or pooled into account inventory still need decisions.

## Two reasons to raid

1. The Visitor arrival automatically triggers a major raid encounter. It cannot
   be banked, stockpiled, or held for later entry. The intended
   atmosphere is a looming celestial threat, inspired by Thread in Pern.
2. Raids also gate progression between the four progression Cycles: Origin,
   Resonance, Conflict, and Singularity.

These are additional purposes, not interchangeable meanings of "cycle". Lunar
periods are recurring calendar events; progression Cycles are advancement stages.
It is not yet decided whether a progression gate uses a distinct raid, a selected
moon raid, or a separate version of the same encounter. Winning any arbitrary
moon raid must not be assumed to advance the account's progression Cycle.

## Current real-time celestial cadence

The implemented clock uses 12 beats per second and 720 beats per calendar day,
so a calendar day currently takes **one real minute at normal speed**. The
implemented celestial periods translate to:

| Event or period | Normal-speed real time | At 8x overdrive |
| --- | --- | --- |
| Runner period | 9 minutes | 1 minute 7.5 seconds |
| Elder period | 21 minutes | 2 minutes 37.5 seconds |
| Two-moon pattern repeat / Split-Light cadence | 63 minutes | 7 minutes 52.5 seconds |
| Twin Dark cadence | 3 hours 9 minutes | 23 minutes 37.5 seconds |
| Visitor return | 22 hours 3 minutes | 2 hours 45 minutes 22.5 seconds |

The 63-minute repeat follows the least common multiple of the 9- and 21-minute
periods. It does not mean both displayed moons become full together. The current
phase display gives Elder a half-cycle offset; named conjunctions are explicit
lore schedules rather than computed visual-phase intersections.

Sources: [WorldClock](../src/fl/primitives/world_clock.hpp),
[MoonCalendar constants](../src/fl/primitives/moon_calendar.hpp),
[phase and countdown implementation](../src/fl/primitives/moon_calendar.cpp),
and [clock/calendar tests](../tests/primitives/test_moon_calendar.cpp).
The underlying lore is in [the moon brainstorm](../brainstorms/a_tale_of_two_moons_and_a_comet.md).

These are active-running intervals at the configured speed, not an offline or
wall-clock catch-up guarantee. At multiplier M, divide normal-speed durations by
M. **The Visitor is the selected recurring raid trigger: 22 hours 3 minutes
at normal speed with the current calendar constants.** Split-Light and Twin Dark
are not the selected raid trigger. Arrival starts the encounter rather than
creating a claimable entry token. Offline behavior is still undecided; it must
not be implemented as a bank of missed raid entries.

The existing countdown is in whole days and reports every named event due at
initial day zero. The confirmed initial raid consumes that day-zero arrival once.
Raid scheduling needs an exact countdown and consumed-occurrence tracking; a
whole-day zero must not repeatedly launch encounters.

## Proposed account-time pause

Design direction requested: pause calendar/time progression for the raiding
account for the duration of its raid. Keep the raid's combat clock running so
ATB, effects, and actions continue. Present the moons' convergence as a real-time
countdown to the next major raid.

Consequences to preserve when implementing this direction:

- Raid combat time and account calendar time must be independently advanceable.
- An account's raid must not freeze unrelated accounts.
- Account calendars can diverge after raids of different lengths.
- At fixed speed, elapsed real time between recurring raid starts includes the
  chosen calendar interval plus time spent with that account's calendar paused.
- Resuming time must not immediately launch the same convergence again.
- The countdown must make the paused state visible rather than imply a fixed
  real-world deadline while combat adds time to it.

Still clarify whether "pause time progression" also freezes recovery, crafting,
other account timers, and offline accrual, and whether overdrive should continue
to shorten the raid calendar interval. The current global clock advances both
calendar time and the beats forwarded to parties; it cannot provide this
account-specific pause as-is.

## Confirmed targeting abstraction

Target selection uses encounter-scoped **iterator-based ranges**, not helpers
that eagerly allocate a candidate list. Each range traverses and filters the
active encounter's participants relative to the actor's side. Support ordinary
range iteration and composition with skill-specific eligibility filters.

| Range | Side relative to actor | Candidate life state |
| --- | --- | --- |
| `FriendlyPossibleTargets(actor)` | Friendly | Alive |
| `FriendlyDeadPossibleTargets(actor)` | Friendly | Dead |
| `EnemyPossibleTargets(actor)` | Opposing | Alive |
| `EnemyDeadPossibleTargets(actor)` | Opposing | Dead |

These are the intended API names; exact C++ signatures will be settled during
implementation. All variants exclude invalid/destroyed entities and entities
outside the encounter. Dead means a valid participating combatant that is dead,
not a null or stale entity handle. The friendly range can include the actor when
its life state matches; skills that exclude self apply that constraint explicitly.
Callers must not enumerate the actor's home party to discover targets.

In a normal fight, friendly candidates are the participating allies in that
encounter. In a raid, they span all five participating parties. For a boss or an
add, friendly candidates are its own encounter allies and opposing candidates
are the participating player side. Home-party identity remains available for
credit, progression, and presentation; it does not limit friendly targeting.

The helpers supply candidates; skill rules then select or filter them. A
single-target heal still picks one eligible ally, while an all-allies effect
covers all eligible allies in the encounter. This scales with participants
without separate party-versus-raid skill implementations. Preserve explicit
skill constraints such as self-only, exclusions, target count, and required
status. Normal damage/healing use living ranges. Skills targeting corpses or
future resurrection use the explicit dead variants plus their own eligibility
rules. Providing a dead-target range does not introduce a resurrection mechanic
or change the no-resurrection-on-raid-entry rule. An actor outside the encounter
has an empty range for every variant.

Use this common path for player skills, monster decision rules, group effects,
and effect execution. Validate targets again when delayed effects execute;
being eligible at scheduling time does not guarantee eligibility later. Ordered
monster-rule conditions must still match the same candidate ultimately selected.
Keep these ranges read-only and accessible through narrow encounter authority.
They borrow encounter storage: do not store an iterator or range beyond its
encounter lifetime, across raid transfer, or in a delayed callback. Delayed work
captures stable entity IDs and revalidates them at execution. Define iterator
invalidation rules explicitly; effects that may structurally change membership
or end the encounter must collect the needed IDs before applying those effects.
That deliberate snapshot is a consumer choice, not eager allocation in every
candidate helper. HP changes during iteration must not permit an invalid or
wrong-life-state candidate to pass eligibility checks when used.

Existing `allied_alive_targets` / `opposing_alive_targets` and `Team` queries
provide useful starting points, but currently resolve through party-owned
encounters. Consolidate the duplicated side-selection logic as shared encounter
ownership is introduced; this contract is planned, not already implemented.

## Targeting implementation progress

The first targeting slice provides `fl::targeting::PossibleTargets` in
`src/fl/targeting/possible_targets.hpp`: four lazy living/dead side-relative
ranges over borrowed encounter participant storage. This is a reusable primitive;
shared account raid ownership is still to be implemented.

Typed status adaptors in `src/fl/targeting/status_filters.hpp` compose with those
ranges. Has/lacks, any-of, and all-of use the same `TargetStatus` vocabulary for
buffs and debuffs, including Poison, Freeze, and Dire Bleed:

```cpp
auto candidates = targets.EnemyPossibleTargets(actor)
    | fl::targeting::HasStatus(reg, TargetStatus::Burn)
    | fl::targeting::LacksStatus(reg, TargetStatus::Shield);
```

Filters narrow the same entity; they never satisfy two conditions on different
targets. These C++20 filter views may cache their first matching iterator, so
recreate ranges after eligibility changes. Deferred/mutating effects can use
`snapshot_targets` to copy IDs deliberately and must still revalidate at use.

## Dedicated raid screen

Confirmed layout direction: a raid has its own screen. Reserve approximately the
upper third for the raid boss and the lower two thirds for all five parties,
each with its own set of lines. All parties must be visible simultaneously;
a party selector, tabs, or scrolling that hides other parties does not meet this
requirement. A wiped party remains visible as part of the shared encounter.

```text
+--------------------------------------------------+
| Raid boss                                        |
| Boss presentation and status                     |  upper 1/3
+--------------------------------------------------+
| Party 1: its own set of character/status lines    |
| Party 2: its own set of character/status lines    |
| Party 3: its own set of character/status lines    |  lower 2/3
| Party 4: its own set of character/status lines    |
| Party 5: its own set of character/status lines    |
+--------------------------------------------------+
```

Root/shell owns navigation; the raid view projects encounter state and reuses
combatant/party widgets where practical. Starting a raid must not depend on the
user opening its screen. Auto-focus for the selected account, simultaneous raids
in other accounts, logs, adds, and minimum terminal dimensions need design work.
At small sizes, define a compact layout or a supported minimum size explicitly;
do not silently violate the all-parties-visible requirement.

## Statistics and achievements

These systems are a requested foundation alongside raids. See
[Statistics and achievements](statistics-achievements-design.md) for the initial
contracts, existing event gaps, open decisions, and implementation milestones.
Raid attempts and outcomes must be recorded once per shared encounter, with
participation credit for every party. A wiped party in a victorious raid must
not turn that account win into an account defeat or reduce its reward count.

## Existing implementation and boundaries to change

- `AccountData` owns parties; each `PartyData` owns its own encounter.
- `EncounterBuilder::thump_it_out` enrolls one party against five enemies.
- `EncounterData` uses one `PartyCtx` and one party's tick source.
- `Team` and Seerin's combatant registration provide variable-sized collections,
  but no shared account encounter lifecycle exists yet.
- Damage/death handling resolves encounters through the character's home party.
- Party exit currently clears scheduled work and destroys its encounter's
  enemies. A wiped party must not trigger that cleanup for a continuing raid.
- Rewards and victory-sensitive skill learning currently have party-scoped
  paths; they must consume the shared outcome for raid participants.
- The existing multi-party wipe test exercises separate encounters, not a raid.

A future design needs an explicit owner for the shared encounter, one tick source,
participant/home-party identity, and narrow encounter authority for combat code.
Do not flatten party ownership or use GrandCentral as a shortcut. Retain party
identity for progression and reward semantics while resolving combat collectively.

## Open decisions for the next discussion

- Current combat is explicitly interrupted by summoning. Settle the mechanics
  of empty-encounter cleanup and recovery/crafting work already in progress.
  HP/death-state preservation, status removal, and skill retention are fixed.
- What happens while the application is closed? Immediate day-zero entry and
  waiting for the next Visitor without retries are confirmed.
- Which timers pause, and how should speed controls affect the countdown?
- How do wiped parties recover after the shared outcome? Is revival possible
  during the encounter?
- How are exclusive drops allocated and equipped, and what happens on defeat?
- Which raids gate which progression transitions, and how are they accessed
  independently of or alongside recurring celestial events?

## Implementation milestones after requirements settle

1. Finalize remaining arrival handoff, calendar, and reward semantics in this file.
2. Introduce shared encounter ownership and participant tracking; test that a
   party wipe preserves the encounter and that only a collective outcome ends it.
   Cover player-only defeat, enemy-only victory, and mutual extinction as defeat.
3. Separate account calendar advancement from raid combat; test pause/resume,
   other-account independence, immediate day-zero entry, single event consumption,
   and no retry before the next Visitor after either victory or defeat.
4. Add account-wide outcome credit and raid-exclusive rewards; verify that wiped
   parties count toward the five-party ten-drop victory allocation.
5. Add progression gates, boss declarations, and the raid/countdown presentation
   once their specific contracts are settled.
6. Integrate resolved raid facts into statistics and achievements; verify one
   account result and consistent credit across all five parties.
7. Protect the intentional summoning loophole with event-driven tests: retain a
   successfully observed skill without winning the old fight; do not award a false
   victory; do not let old callbacks revoke it on a later raid wipe; preserve
   HP/death state, clear old statuses, and prevent old enemies/actions from
   continuing to attack.
8. Test the common target helpers in both normal and raid encounters, including
   cross-party healing/buffs, enemy-side symmetry, all four living/dead variants,
   empty ranges, invalid actors/entities, self eligibility, iterator lifetime and
   invalidation, delayed target invalidation, and unchanged single-target versus
   group rules.

No raid runtime changes are part of this initial design documentation pass.
