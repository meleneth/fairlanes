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
- Victory requires all enemies in the encounter to be dead. Defeat requires
  all participating characters to be dead. A boss dying while its adds remain
  alive is not sufficient for victory under this rule.
- Victory is all or nothing for the account. Every participating party receives
  victory credit, including a party whose entire membership died before the win.
- If the last enemies and characters die together, outcome precedence remains
  an open decision; do not let callback ordering decide it accidentally.

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
initial day zero. Raid scheduling needs a deliberate first-event policy and an
exact countdown; a whole-day zero must not repeatedly launch encounters.

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

- At automatic Visitor arrival, what happens to parties already farming,
  recovering, or otherwise occupied? Do existing HP and statuses carry in?
- Are retries immediate, limited, or deferred until another convergence?
- Is the initial convergence immediate, delayed by a full interval, or tied to
  account readiness? What happens while the application is closed?
- Which timers pause, and how should speed controls affect the countdown?
- Do heals, buffs, and group attacks target a home party or the full raid team?
- How do wiped parties recover after the shared outcome? Is revival possible
  during the encounter?
- How are exclusive drops allocated and equipped, and what happens on defeat?
- Which raids gate which progression transitions, and how are they accessed
  independently of or alongside recurring celestial events?
- How is simultaneous extinction resolved?

## Implementation milestones after requirements settle

1. Finalize arrival handoff, calendar, retry, targeting, and reward semantics in this file.
2. Introduce shared encounter ownership and participant tracking; test that a
   party wipe preserves the encounter and that only a collective outcome ends it.
3. Separate account calendar advancement from raid combat; test pause/resume,
   other-account independence, first convergence, and single event consumption.
4. Add account-wide outcome credit and raid-exclusive rewards; verify that wiped
   parties count toward the five-party ten-drop victory allocation.
5. Add progression gates, boss declarations, and the raid/countdown presentation
   once their specific contracts are settled.
6. Integrate resolved raid facts into statistics and achievements; verify one
   account result and consistent credit across all five parties.

No raid runtime changes are part of this initial design documentation pass.
