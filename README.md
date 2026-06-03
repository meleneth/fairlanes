# Fairlanes

One day, a Field Mouse thumped me. That part is still true.

Fairlanes is a C++20 terminal autobattler / Progress Quest-like game experiment. The machine drives the parties; the interesting work is in the simulation loop, combat timing, skill observation, status effects, loot, and the UI that lets you watch the little world grind forward.

## Current Shape

The current runtime is built around:

- **EnTT ECS** for entities and components.
- **SML / PartyLoop** for party progression through farming, combat, town recovery, and gearing.
- **Seerin ATB** for combat timing, readiness, active turns, freeze/thaw, and scheduled action work.
- **eventpp through `seerin::VariantBus`** for the live party, combatant, beat, and ATB event buses.
- **FTXUI** for the terminal UI.

The important combat spine is:

```text
seerin::Beat -> PartyTick -> ATB -> BecameActive -> skill scheduling -> FinishedTurn
```

In practice:

1. `GrandCentral` emits global `seerin::Beat` events.
2. `PartyData` forwards beats into the party loop.
3. `PartyLoop` emits `PartyTick` while the party is in combat.
4. `EncounterData` converts party ticks into ATB beats.
5. `AtbEngine` chooses one active combatant at a time.
6. `SkillSequencer` schedules visuals, effects, damage, status work, and turn completion.

## What Exists Now

Fairlanes currently has:

- 8 accounts, each with parties and party members.
- Automated encounter creation and ATB-driven combat.
- Skill selection and timed skill sequences.
- Thump-like attacks, Flame Strike, Flame Wave, Mercyburst, Observe, Poison, Cold Snap, Eviscerate, and Flee behavior.
- Poison, Freeze, and Dire Bleed status effects with cleanup on death/combat exit.
- XP, level gains, town recovery, loot requests, starting festival drops, and basic gear maintenance.
- Account/party battle views, combatant projections, logs, inventory rows, and console/debug commands.

The codebase has moved past speculative event/bus scaffolding. If a type is not part of the current runtime, it should not be treated as architecture.

## Build And Test

Use the existing debug build tree for normal local checks:

```sh
cmake --build build-linux-debug
ctest --test-dir build-linux-debug --output-on-failure
```

Fresh build:

```sh
cmake -S . -B build-linux-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-linux-debug
ctest --test-dir build-linux-debug --output-on-failure
```

Formatting target:

```sh
cmake --build build-linux-debug --target clang-format
```

## Running

```sh
cmake --build build-linux-debug
./build-linux-debug/fairlanes
```

Function keys `F1` through `F8` switch between accounts.

Tracy profiling is integrated. If Tracy is enabled in the build, the game may open a profiling connection when it runs.

## Reading The Code

Hosted docs: https://meleneth.github.io/fairlanes/

Start here:

- `docs/tour/index.md`
- `docs/tour/architecture.md`
- `docs/tour/event-flow.md`
- `docs/tour/event-reference.md`
- `docs/tour/fairlanes/context.md`
- `docs/tour/seerin/combat_engine.md`

Useful source entry points:

- `src/fl/grand_central.cpp`
- `src/fl/primitives/party_data.cpp`
- `src/fl/fsm/party_loop.cpp`
- `src/fl/primitives/encounter_data.cpp`
- `src/sr/atb_engine.cpp`
- `src/fl/skills/skill_sequence.cpp`
- `src/fl/events/party_bus.hpp`

## Design Direction

The gameplay loop is:

```text
go to an area -> fight -> get loot/xp -> return to town -> recover -> maintain gear -> repeat
```

Skill learning is still one of the central images: a party member sees a creature use a skill, studies it through Observe, and may carry that skill forward if the fight is won.

Longer-term lore and item ideas, including Cycles, filigree armor, and Darrin-mark, live in the docs and brainstorm files until the code promotes them into runtime systems.

## Contributing Notes

Keep UI widgets as projections. Gameplay authority belongs in systems, state machines, context-scoped logic, and data models.

Use the narrowest context that fits. `GrandCentral` owns the world, but normal code should not reach for it.

Prefer concrete runtime events over aspirational stubs. If a new event exists, it should have a real producer, a real consumer, or a very clear near-term integration path.
