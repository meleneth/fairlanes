# Event Reference

Back to [Tour Home](./index.md)

This page documents the event payloads that currently exist in Fairlanes and, most importantly, what triggers them. It focuses on the two active event styles in the codebase:

- SML state-machine events, which are passed with `process_event(...)`.
- Variant/eventpp buses, mostly through `seerin::VariantBus<std::variant<...>>` and a few direct `eventpp` dispatchers.

Some payloads are defined but do not yet have production emitters. Those are listed on purpose: they are part of the current event surface and should be audited before new combat abilities, skills, or enemies build on them.

## Main Runtime Flow

The active combat event chain is currently:

1. `GrandCentral::main_loop(...)` emits global `seerin::Beat` on the root `seerin::BeatBus` each simulation beat.
2. `PartyData::hook_to_beat(...)` listens to the global beat, forwards `seerin::Beat` to the party beat bus, and calls `PartyLoopMachine::beat_event()`.
3. `PartyLoopMachine::beat_event()` sends `fl::fsm::NextEvent` into the party SML machine.
4. `PartyLoop::Ops::combat_tick(...)` emits `fl::events::PartyTick` while the party remains in combat.
5. `EncounterData` listens for `PartyTick` and emits `seerin::Beat` into the encounter ATB engine.
6. `AtbEngine` turns ATB beats into internal `BeatTick`, eventual `BecameReady`, and then `BecameActive` when no other combatant is active.
7. `EncounterData` listens for `BecameActive`, chooses a target and skill, then schedules the skill sequence.
8. The scheduled sequence applies effects and eventually emits `seerin::FinishedTurn` for the attacker.

## SML Events

### `fl::fsm::NextEvent`

Defined in `src/fl/fsm/party_loop.hpp`.

Triggered by `PartyLoopMachine::beat_event()` once per forwarded party beat. The caller is `PartyData::hook_to_beat(...)`, which runs after a global `seerin::Beat` is received from `GrandCentral`.

Handled by `fl::fsm::PartyLoop`:

- `Idle -> Farming`: starts the farming/combat loop.
- `Farming -> Farming` with `in_combat` guard: emits a party combat tick.
- `Farming -> CombatIdle` without `in_combat`: leaves combat and returns to town.
- `CombatIdle -> Gearing`: runs gear maintenance next.
- `Dead -> Fixing`: enters town recovery.
- `Fixing -> Fixing` while recovery is still active: decrements the town penalty.
- `Fixing -> Gearing` when recovery is done: revives party members.
- `Gearing -> Idle`: returns to idle loop.

### `fl::fsm::PartyWipedEvent`

Defined in `src/fl/fsm/party_loop.hpp`.

Triggered by `PartyLoopMachine` when the party bus emits `fl::events::PartyWiped`. The machine subscribes to the party bus in `PartyLoopMachine::Impl` and converts the bus event into this SML event.

Handled by `fl::fsm::PartyLoop` from every current state. It transitions to `Dead`, where `enter_dead(...)` logs the wipe recovery, leaves combat, and starts the town penalty.

### `seerin::BeatTick`

Defined in `src/sr/atb_events.hpp` as an ATB FSM-only event.

Triggered inside `AtbEngine::on_beat(...)` for each registered combatant that is valid and allowed to charge. It is not emitted on a public bus.

Handled by `seerin::AtbMachine`:

- In `Charging`, it increments the entity's `AtbCharge`.
- If the next beat fills the charge bar, it also emits `seerin::BecameReady` and transitions to `Ready`.

### `seerin::FinishedTurn` as an ATB FSM event

`FinishedTurn` is also a public `AtbInEvent`, but inside `AtbEngine` it is forwarded into each combatant's SML machine when appropriate.

Triggered internally when:

- `AtbEngine::on_finished_turn(...)` receives public `seerin::FinishedTurn` for the active combatant.
- `AtbEngine::on_beat(...)` sees a combatant that cannot charge, usually because it is dead, and resets that combatant.

Handled by `seerin::AtbMachine` in `Ready`: resets charge to zero and returns to `Charging`.

## Global And Party Beat Events

### `seerin::Beat`

Defined in `src/sr/atb_events.hpp`.

Used as the currently active beat payload for the root `seerin::BeatBus`, party beat bus, and ATB input bus.

Triggered by:

- `GrandCentral::main_loop(...)` in UI mode and headless mode, once per simulation beat, on the global beat bus.
- `PartyData::hook_to_beat(...)`, which forwards each global beat to that party's `party_beat_bus_`.
- `EncounterData`, which converts each `fl::events::PartyTick` into an ATB input beat with `atb_in().emit(seerin::Beat{})`.

Observed by:

- `PartyData::hook_to_beat(...)` from the global bus.
- `AtbEngine`, through `AtbInEvent`, to advance scheduled work and ATB charge.

### `fl::events::Beat` and `fl::events::BeatPulse`

Defined in `src/fl/events/beat.hpp` and `src/fl/events/beat_bus.hpp`.

`fl::events::Beat` carries timestamp, delta, and beat index. `BeatPulse` wraps it for `fl::events::BeatBus`.

Production trigger status: defined, but no production emitter was found in the current code. The active runtime currently uses `seerin::Beat` instead.

## Party Bus Events

`fl::events::PartyEvent` is a variant bus payload defined in `src/fl/events/party_bus.hpp`.

### `PartyCreated`

Production trigger status: defined, but no production emitter was found. Party construction currently writes logs directly rather than emitting this event.

### `MemberJoined`

Production trigger status: defined, but no production emitter was found.

### `MemberLeft`

Production trigger status: defined, but no production emitter was found.

### `PartyWiped`

Triggered by `TakeDamage::commit(...)` after a player dies and `PartyData::all_members_dead()` returns true for that player's party.

Observed by:

- `PartyLoopMachine`, which translates it into `fl::fsm::PartyWipedEvent`.
- Tests and any party-bus subscribers.

### `PartyLeftCombat`

Triggered by `PartyData::leave_combat()` when an active encounter exists and is being torn down.

Observed by `DireBleedSystem`, which clears Dire Bleed status, visual overrides, and pending target-owned scheduled work when combat ends.

### `PartyGainedXP`

Production trigger status: defined, but no production emitter was found. XP is currently granted through `GrantXPToParty::commit(...)` without this event.

### `PartyHealed`

Production trigger status: defined, but no production emitter was found. Party recovery currently happens through `PartyData::revitalize_members()` without this event.

### `PartyTick`

Triggered by `PartyLoop::Ops::combat_tick(...)` whenever the party SML machine receives `NextEvent` while `Farming` and `in_combat` is true.

Observed by `EncounterData`, which turns the party tick into `seerin::Beat` on the encounter ATB input bus.

### `PreAttack`

Production trigger status: defined, but no production emitter was found. Current skills call attack/effect systems directly from scheduled callbacks.

### `PostAttack`

Production trigger status: defined, but no production emitter was found. Current damage resolution does not emit a post-attack party event.

### `PlayerDied`

Triggered by `TakeDamage::commit(...)` when a defender was alive before damage and is not alive after damage, and the defender is a party member.

Observed by `DireBleedSystem`, which clears Dire Bleed if the affected player dies.

## ATB Bus Events

`seerin::AtbInEvent` and `seerin::AtbOutEvent` are variant bus payloads defined in `src/sr/atb_events.hpp`.

### `seerin::AddCombatant`

Triggered by:

- `EncounterBuilder::thump_it_out()` for each defending party member added to a new encounter.
- `EncounterBuilder::add_to_enemy_team(...)` for each spawned enemy.
- Tests that add combatants directly to an encounter or ATB engine.

Handled by `AtbEngine::on_add(...)`, which registers the entity as an ATB combatant and creates/replaces its ECS `AtbCharge` component.

### `seerin::FinishedTurn`

Triggered by:

- `EncounterData` immediately when an active combatant has no valid target.
- `SkillSequencer` finish callbacks after a scheduled skill sequence completes, such as Thump-like skills at beat 71, Eviscerate at beat 34, and Observe at beat 12.

Handled by `AtbEngine::on_finished_turn(...)`, which clears the active combatant and forwards the event to that combatant's ATB state machine so charge resets.

### `seerin::BecameReady`

Triggered by `seerin::AtbMachine` when a `BeatTick` fills a combatant's ATB charge.

Handled by `AtbEngine::on_became_ready(...)`, which enqueues the entity in the ready queue.

### `seerin::BecameActive`

Triggered by `AtbEngine::pump_ready_queue()` when there is no current active combatant and a ready combatant can act.

Observed by `EncounterData::innervate_event_system()`. The handler chooses a target and skill, then schedules the selected skill sequence. If no valid target exists, it emits `FinishedTurn` immediately.

## Seerin Encounter Events

Defined in `src/sr/encounter_events.hpp` as `seerin::EncounterEvent`.

### `seerin::ActionRequested`

Production trigger status: defined, but no production emitter was found. Current active combat flow schedules skills from `BecameActive` directly instead of emitting this intent event.

### `seerin::ApplyEffect`

Production trigger status: defined, but no production emitter was found. Current skill effects call ECS systems directly from scheduler callbacks.

`EncounterEvent` also includes `Beat`, `AddCombatant`, and `BecameReady`; those are documented above under the active ATB flow.

## Battle Bus Events

`fl::events::BattleEvent` is a variant bus payload defined in `src/fl/events/battle_bus.hpp`.

### `StartCombat`

Triggered by `BattleBus::start_combat(encounter)`.

Production trigger status: the helper exists and is tested, but no production caller was found in the current code.

### `BattleTick`

Triggered by `BattleBus::tick(dt)`.

Production trigger status: the helper exists and is tested, but no production caller was found in the current code.

### `EndCombat`

Triggered by `BattleBus::end_combat(encounter, reason)`.

Production trigger status: the helper exists and is tested, but no production caller was found in the current code.

## Account Bus Events

`fl::events::AccountEvent` is a variant bus payload defined in `src/fl/events/account_bus.hpp`.

### `AccountCreated`

Production trigger status: defined, but no production emitter was found.

### `AccountDeleted`

Production trigger status: defined, but no production emitter was found.

### `AccountRenamed`

Production trigger status: defined, but no production emitter was found.

### `AccountTick`

Production trigger status: defined, but no production emitter was found.

## Logging Events

### `fl::primitives::LogEvent`

Defined in `src/fl/primitives/logging.hpp` and dispatched through direct `eventpp::EventDispatcher` using `LogKey::Message`.

Triggered by `fl::primitives::Logger::log(...)` and its convenience methods: `trace`, `debug`, `info`, `warn`, and `error`.

Observed by `FancyLogSink`, which appends matching log messages into a `FancyLog` view and removes its eventpp listener in its destructor.

### `fl::events::LogEvent`

Defined separately in `src/fl/events/log_event.hpp`.

Production trigger status: defined, but no production emitter was found. The active logging path uses `fl::primitives::LogEvent`.

## Timer And Scheduler Events

### `fl::events::TimerEvent`

Defined in `src/fl/events/timed_event_queue.hpp` and stored in a direct `eventpp::EventQueue`.

Triggered by `TimedEventQueue::schedule_in(...)` and `TimedEventQueue::schedule_at(...)`, then executed when `TimedEventQueue::process_events(dt)` advances the queue window past the event's `when` time.

Production trigger status: the queue is tested, but no production caller was found.

### `seerin::TimedScheduler<AtbOutEvent>::EmitEvent`

Defined in `src/sr/timed_scheduler.hpp` as a scheduled wrapper around a typed event variant.

Triggered by `schedule_at(...)`, `schedule_in(...)`, and `schedule_in_beats(...)`. Emitted when `TimedScheduler::on_beat()` advances far enough.

Current production status: the generic typed-event path exists, but the active skill code mostly uses smelly callbacks instead of typed scheduled events.

### `seerin::TimedScheduler<AtbOutEvent>::SmellyCallback`

Defined in `src/sr/timed_scheduler.hpp` as a scheduled callback with a note and optional owner entity.

Triggered by `schedule_smelly_at(...)`, `schedule_smelly_in(...)`, `schedule_smelly_in_beats(...)`, and their owner-aware variants. Executed when `TimedScheduler::on_beat()` advances far enough.

Used by the current implemented skills and statuses:

- Thump-like skills schedule attacker color pulses, defender hit color, damage application, and turn completion.
- Eviscerate schedules slash/wound visuals, Dire Bleed application, and turn completion.
- Observe schedules a log message and turn completion.
- Dire Bleed schedules repeated bleed ticks until cleanup conditions remove the status.

## Animation Events

### `fl::events::AnimationFinished`

Defined in `src/fl/events/animation_finished.hpp` with `AnimBus` as direct `eventpp::EventDispatcher<int, void(const AnimationFinished &)>`.

Production trigger status: defined, but no production emitter was found.

## Older Or Unwired Combat Events

Defined in `src/fl/events/combat.hpp`.

### `fl::events::Tick`

Production trigger status: defined, but no production emitter was found. Current beat/tick flow uses `seerin::Beat` and `fl::events::PartyTick`.

### `fl::events::PlayerCommandAttack`

Production trigger status: defined, but no production emitter was found. Current combat is automated through ATB activation and skill selection.

### `fl::events::AttackResolved`

Production trigger status: defined, but no production emitter was found. Current damage resolution mutates ECS state directly and emits death/wipe party events only when applicable.

## Legacy Party Combat Bus

Defined in `src/fl/combat/party_bus.hpp`.

### `fl::combat::PartyInEvent`

Variant currently containing `seerin::Beat`.

Production trigger status: defined, but no production emitter was found. Current party beat forwarding uses `seerin::BeatBus` directly in `PartyData`.

### `fl::combat::PartyOutEvent`

Variant currently containing `seerin::BecameReady`.

Production trigger status: defined, but no production emitter was found.

## Ready Queue Data

### `fl::events::Decision`

Defined in `src/fl/events/ready_queue.hpp` and stored by `fl::events::ReadyQueue`.

Production trigger status: defined, but no production caller was found. The active ready queue is currently `AtbEngine::ready_queue_`, which stores `entt::entity` values and emits `seerin::BecameActive` when one is selected.

## Audit Notes For Combat Content

The current implemented combat content is driven less by domain events and more by scheduled callbacks:

- Skills are selected when `BecameActive` fires.
- Skill timing is represented by `TimedScheduler` callback notes.
- Damage is applied directly by skill callbacks through `TakeDamage::commit(...)`.
- Death and party wipe are the main gameplay outcomes currently published to the party bus.

That means the next audit should decide whether ability milestones such as action requested, effect applied, attack resolved, status applied, status ticked, and status cleared should become typed events rather than callback notes and direct system calls.
