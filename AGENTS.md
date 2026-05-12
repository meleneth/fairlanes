# Local Clean Code Conventions

- Keep UI components focused on rendering, event routing, and view state. Command parsing and command behavior should live in a dedicated class, with widgets wiring UI effects through small callbacks.
- Prefer data-oriented model classes for account, party, inventory, and encounter state. Views should read from those models rather than duplicating state.
- When adding a new screen, keep navigation decisions at the root/shell layer and keep each screen responsible for one presentation concern.
- Reuse existing widgets for repeated presentation, especially combatants, party status, logs, and inventory-style rows.
- Add narrow seams only where they remove real coupling. Avoid burying application behavior inside FTXUI render methods.

# Local Build

- Use the existing `build-linux-debug` tree for normal compile and test checks: `cmake --build build-linux-debug` and `ctest --test-dir build-linux-debug --output-on-failure`.

# Local Debugging

- In this dev-container + FTXUI setup, GDB output is usually swallowed by the alternate screen buffer and is not a reliable workflow. Do not spend time on GDB-first debugging here.
- Prefer reproducible checks instead: targeted test runs, temporary log breadcrumbs, assertions, and sanitizer builds.

# Fairlanes Agent Notes

These notes summarize the repo Markdown as of 2026-05-11. Keep them current when
the docs change.

## Project Shape

Fairlanes is a C++20 CMake project for an autobattler / Progress Quest-like game
experiment. The current architecture is intentionally built around:

- EnTT ECS for what exists: entities and components.
- SML / Seerin / PartyLoop state machines for how things progress.
- eventpp-backed project bus types for how systems communicate.
- FTXUI widgets for the terminal UI surface.

The design priority is clarity of structure first, then gameplay complexity on
top.

## Build And Test

Typical local build:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```



Formatting target mentioned by README:

```sh
cmake --build . --target clang-format
```

Tracy profiling is integrated. Windows may show a network access warning when the
game runs because Tracy opens a connection for live profiling.

## Architectural Rules

Read behavior in this order:

1. Data: ECS entities and components.
2. Flow: FSM / Seerin / PartyLoop.
3. Signals: event buses, dispatchers, and listeners.

GrandCentral owns the world and drives the main loop, but normal code must not
reach for it. Treat it as the root owner only. If code wants GrandCentral, it
probably needs a narrower context instead.

Context objects are central, intentionally subtle API objects. They are not just
bundles of convenience references. They encode what a piece of code is allowed
to see, which log it writes to, and how far through the world graph it can move.

Use the narrowest context that fits:

- EntityCtx for entity-local work.
- BuildCtx for setup/construction.
- PartyCtx for party-scoped gameplay and party bus access.
- AccountCtx for account-level navigation and scoped context creation.
- AttackCtx for transient attacker/defender/damage state.
- WorldCoreCtx-style code when only registry, RNG, and log are needed.

Contexts are pointer-backed but expose reference-returning accessors. Treat the
raw pointer storage as an implementation detail and the context type as the
authority boundary.

Contexts frequently morph into narrower or adjacent context types to match API
needs:

- `AccountCtx::party_context(...)` creates party-scoped authority from an
  account.
- `AccountCtx::entity_context(...)` creates entity-scoped authority using the
  account log.
- `PartyCtx::entity_context(...)` creates entity-scoped authority using the party
  log.
- `PartyCtx::build_context()` narrows setup code to registry, RNG, and party log.
- `AttackCtx::make_attack(...)` creates transient combat authority from a
  `PartyCtx`.
- `EntityCtx::expect_party_ctx()` rehydrates a party context from an entity that
  really has `IsParty`.

The log attached to a context matters. There are several real `FancyLog`
instances: the master GrandCentral log, one per account, and one per party.
`ctx.log()` is therefore not a generic global logger. It is the current scope's
human-facing trace surface. When changing context conversions, preserve which log
the new context should write to.

## Ownership And Handle Stability

The object graph deliberately uses owning containers and pointer-stable storage
so contexts can stay lightweight without becoming dangling traps.

Current ownership shape:

- `GrandCentral` owns `std::deque<AccountData> accounts_`.
- `AccountData` owns `std::deque<PartyData> parties_`, an account `FancyLog`,
  and an account bus.
- `PartyData` owns `std::deque<MemberData> members_`, a party `FancyLog`, a
  `PartyCtx`, a `PartyLoopMachine`, optional `EncounterData`, and a party bus.
- `PartyData::party_ctx_` points back into the account/party/world objects it was
  built from.
- `GrandCentral` owns the master `FancyLog` through `std::unique_ptr` and wires a
  `FancyLogSink` to the `LogBus`.

The use of `std::deque` is important. Code stores references and pointers into
the account, party, and member collections through contexts, components, and UI
objects. Do not casually replace these deques with vectors: vector reallocation
would invalidate handles that the current API relies on being stable.

The `std::unique_ptr<FancyLog>` members are also part of the stability story:
moving an `AccountData` or `PartyData` should not move the actual log object that
existing contexts point at.

When documenting or changing object APIs, spell out:

- who owns the object
- whether references/pointers/contexts to it are expected to remain stable
- which context type may expose it
- which log scope it uses
- whether moving the owner preserves the pointee's address

Do not store long-lived raw pointers to temporary contexts. Store or derive a
context from stable owner data, or make lifetime explicit with RAII.

## ECS Guidance

Components should be small, explicit, focused data. Do not turn components into
policy-heavy objects or "bags of whatever."

Systems describe what happens to component data. If logic reads components,
modifies components, or reacts to gameplay events, it probably belongs in an ECS
system rather than inside a component.

Good entry points:

- `src/fl/ecs/components/stats.hpp`
- `src/fl/ecs/components/color_override.hpp`
- `src/fl/ecs/systems/take_damage.cpp`
- `src/fl/ecs/systems/grant_xp_to_party.cpp`

Watch for implicit ordering between systems. If correctness depends on ordering,
make it explicit in code and docs.

## Event Guidance

Fairlanes uses eventpp mostly through project-level bus types, not raw eventpp
usage everywhere. Search by domain terms first:

- `PartyBus`
- `AccountBus`
- `PartyEvent`
- `AccountEvent`
- `appendListener`
- `dispatch(`

Mental model:

```text
event enum -> dispatch() -> appendListener()
```

Events should be clearly named, specific, narrow, and meaningful. Avoid vague
events such as "updated", "changed", or "thing_happened" unless the domain really
supports that meaning.

Use events when one action should be observable by multiple consumers or when the
producer should not know every downstream reaction. Do not use events just to
avoid making a direct dependency decision.

Subscription lambdas are central to the event model. If a lambda captures
`this`, the subscribing object must own the listener handle and remove it before
the captured object can die. `FancyLogSink` and beat forwarding in `PartyData`
are examples worth studying before adding more subscriptions.

## FSM And Combat Flow

FSMs are first-class structure in this repo. When reading or changing FSM-heavy
code, identify:

1. Current state.
2. Triggering event.
3. Guards.
4. Transition target.
5. Transition actions.
6. Events emitted by the transition.

`PartyLoop` is a main state machine. `NextEvent` acts like its general advance
event. The `Farming` state is especially important because it may redirect to
town work, process combat in place, or fall through depending on guards.

Seerin owns the ATB combat timing model: time progression, readiness, action
scheduling, and who acts next. If a change touches combat timing or turn
readiness, it probably belongs in or near Seerin.

Important Seerin invariant from the docs: only one combatant is active at a time.
When that combatant signals `seerin::FinishedTurn{entity_id}`, the next ready
combatant, if any, becomes active.

`entt::entity{}` is the null sentinel (`entt::null`), not a real entity.

## Combat Loop And Timed Intent

The combat loop is fed by several state/event layers. Keep the chain visible
when changing it:

1. `GrandCentral` emits global `seerin::Beat` events.
2. `PartyData::hook_to_beat(...)` forwards each global beat to the party beat bus
   and calls `PartyLoopMachine::beat_event()`.
3. `PartyLoopMachine` processes `NextEvent` through the SML `PartyLoop`.
4. `PartyLoop::Farming` may create a `thump_it_out()` encounter on entry.
5. While in combat, `PartyLoop::combat_tick(...)` dispatches
   `PartyEvent::Tick`.
6. `EncounterData` listens for party ticks and emits ATB `Beat` events.
7. `AtbEngine::on_beat(...)` advances the timed scheduler, ticks each
   combatant's `AtbMachine`, and pumps ready combatants into active turns.
8. `EncounterData` listens for `BecameActive` and schedules the action sequence.

This layered flow is intentional: party lifecycle, encounter lifecycle, ATB
readiness, and visual/action timing are separate concerns that meet through
events and contexts.

`TimedScheduler` uses `uWu` time. One beat is `UWU_PER_BEAT == 80`; the default
ATB full charge is `4800`, which is 60 beats. `on_beat()` advances scheduler time
by one beat and runs due work.

There are two scheduler styles:

- Preferred long-term shape: `schedule_at`, `schedule_in`, and
  `schedule_in_beats` emit typed events.
- Current practical shape: `schedule_smelly_at`, `schedule_smelly_in`, and
  `schedule_smelly_in_beats` store callbacks with grep-able notes.

The "smelly" callback API is a deliberate transitional tool, not random mess. It
currently captures intent over time better than scattered immediate side effects,
especially for animation-like sequencing.

Best current example: `EncounterData::schedule_thump_sequence(...)`.

- attacker red pulse #1 fades from beat 10 to 20
- attacker red pulse #2 fades from beat 30 to 40
- defender yellow hit fades from beat 50 to 70
- damage applies at beat 60
- the turn finishes at beat 71

Dire Bleed is the current best bespoke example for eventful status effects and
self-cleaning subscriptions. Study `EncounterData::schedule_eviscerate_sequence`,
`EncounterData::schedule_dire_bleed_tick`, `EncounterData::bind_dire_bleed_cleanup`,
and `EncounterData::clear_dire_bleed` in `src/fl/primitives/encounter_data.cpp`,
plus `src/fl/ecs/components/dire_bleed.hpp`, before adding another lingering
combat status. It shows the pattern of scheduling repeated beat work, applying
an ECS status and HP-bar visual override, listening for both player death and
left-combat events, and letting either event remove both subscriptions and
clear the status/visual marker.

The fade helper, `schedule_reek_fade(...)`, schedules a sequence of
`ColorOverride` updates using `ftxui::Color::Interpolate(...)`, then schedules a
clear on `end_beat + 1`. This is the best current pattern for "show the intent
over time, then clean up the visual marker."

When adding timed behavior:

- prefer one named scheduling function that describes the whole sequence
- give every smelly callback a useful note string
- capture only stable handles: usually entity ids and pointers/references whose
  owner outlives the scheduled work
- schedule cleanup explicitly
- keep the real gameplay effect at a clear beat in the sequence
- add tests for ordering, cleanup, and turn completion when possible

Test note: the timed scheduler tests in `tests/seerin/test_time_scheduler.cpp`
currently document useful expectations but are commented out. Re-enabling or
replacing them with compiling coverage should be a priority before this timing
layer grows much further.

## UI Guidance

Widgets project state; they are not gameplay authorities.

- `Combatant` shows one participant in combat and is a useful ECS-to-UI slice.
- `AccountBattleView` is the main account-level battle surface: combatants plus
  combat log.
- `FancyLog` renders game output with lightweight `[tag](text)` markup.

`FancyLog::append_markup(std::string_view utf8_line)` parses `[tag](text)`.
Unknown tags must degrade gracefully by rendering plain text rather than losing
content.

If gameplay changes what combatants are or what combat does, expect related UI
updates.

FTXUI deserves explicit care. This repo's UI is not just a debug afterthought:

- `RootComponent` owns screen switching and the console overlay.
- `AccountBattleView` renders party combat rows and account/party log panes.
- `LogWall` renders the master log plus account and party logs together.
- `ConsoleOverlay` writes commands into the currently selected `FancyLog`.
- `Combatant` is a compact projection of ECS state.

The Lospec500 palette is available through `fl::lospec500`:

- `raw_colors()` and `color_at(i)` expose `ftxui::Color`.
- `colors()` and `at(i)` expose decorators.
- `on_not_black(color)` applies foreground color over palette index 0.

Keep palette usage intentional. The current style map in `FancyLog` uses tagged
semantic colors such as `player_name`, `enemy_name`, `xp`, `level`, `error`,
`warn`, and `ability`. The next useful abstraction is likely a "pen" or color
grouping layer so code can ask for semantic roles instead of scattering numeric
palette indices.

## C++ Practices From The Docs

Prefer RAII when cleanup must happen reliably at scope exit. This applies beyond
memory: locks, files, sockets, temporary registrations, event subscriptions,
transactions, and scoped state changes.

Be careful with lambdas in event and FSM code. The capture list is part of the
design. Avoid capturing `this` or references when a callable may be stored,
deferred, or subscribed beyond the captured object's lifetime.

If a lambda starts carrying major policy, promote it into a named function,
object, or system that can be tested directly.

Lambdas are not incidental in this codebase. They show up in event subscriptions,
FTXUI render/event handlers, state machine actions, and local ECS/UI transforms.
Small lambdas are fine, but keep capture lists explicit and review lifetimes as
part of the API. A stored lambda is an ownership relationship wearing a tiny hat.

## Testing Guidance

Tests use Catch2. Tests should clarify behavior and protect design boundaries.
Prefer focused tests that read like behavioral claims:

- given this state
- when this action occurs
- then this outcome follows

FSM-heavy code should cover valid transitions, invalid transitions, guards,
transition side effects, and emitted events.

Event-heavy code should make producers, event contracts, subscribers, and state
changes visible.

Avoid tests that accidentally exercise half the system, rely on unstable timing,
or encode implementation trivia as the contract.

The docs note that there may currently be three failing tests on the author's
machine. Do not assume a failing suite is caused by your change without checking
the baseline.

Improving the test suite is a standing priority. The architecture will only get
more complex, so favor small tests that pin down invariants before the system
gets clever:

- context conversion preserves registry, RNG, entity identity, bus access, and
  the intended log scope
- account/party/member storage keeps references and context handles stable across
  ordinary construction and moves
- event listener RAII removes callbacks before captured objects die
- lambda-heavy flows have tests around the externally visible behavior
- FancyLog keeps its ring-buffer cap, parses markup safely, and degrades unknown
  tags to plain text
- FTXUI projections stay read-only with respect to gameplay authority

## Game And Lore Context

Current design direction:

- 8 accounts.
- 5 parties per account.
- 5 players per party.
- 5 skills per player.
- Autobattler / automated progression loop.
- Field Mouse using Thump is a key early skill-learning image.

The progression loop is roughly: go to an area, fight, get loot, return to town,
heal, craft/maintain gear, progress skills, get consumables, repeat.

Major progression theme: the world evolves through cycles. Early game starts with
mundane weapons, then later introduces magic, modern weapons, and cyber/lazer
weapons. Older defenses can become invalidated by later chapters.

Armor filigree is experience-bound armor progression. Gear has memory by Cycle:

```cpp
effective_filigree = armor.filigree[current_cycle]
```

Only filigree relevant to the current Cycle is active. Higher-tier armor has
higher caps and base absorption; lower-tier armor masters quickly but caps out
earlier. Upgrades extend potential rather than replacing earned experience.

Item slots currently described:

- Chest, helm, boots, gloves, belt.
- Cape.
- Necklace.
- Two rings.

Currency mentioned: Darrin-mark.

The brainstorm file describes a world cadence based on two moons and a visitor:
Runner, Elder, and Visitor cycles. Treat it as lore/design material, not settled
engine policy, unless code or docs promote it.

## Documentation Map

Start with:

- `README.md`
- `docs/tour/index.md`
- `docs/tour/architecture.md`
- `docs/tour/event-flow.md`
- `docs/tour/development.md`

Foundation docs:

- `docs/tour/foundations/ecs.md`
- `docs/tour/foundations/event-driven.md`
- `docs/tour/foundations/fsm.md`
- `docs/tour/foundations/raii.md`
- `docs/tour/foundations/unit-testing.md`
- `docs/tour/foundations/lambdas.md`

Fairlanes-specific docs:

- `docs/tour/fairlanes/grand_central.md`
- `docs/tour/fairlanes/context.md`
- `docs/tour/fairlanes/systems.md`
- `docs/tour/seerin/combat_engine.md`
- `docs/tour/fairlanes/widgets/combatant.md`
- `docs/tour/fairlanes/widgets/account_battle_view.md`
- `docs/tour/fairlanes/widgets/fancy_log.md`

Library notes:

- `docs/tour/libraries/entt.md`
- `docs/tour/libraries/catch2.md`
- `docs/tour/libraries/boost_ext_sml.md`
- `docs/tour/libraries/eventpp.md`

Lore/design brainstorm:

- `brainstorms/a_tale_of_two_moons_and_a_comet.md`
