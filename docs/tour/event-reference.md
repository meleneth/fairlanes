# Event Reference

Back to [Tour Home](./index.md)

This page documents the event payloads that currently exist in Fairlanes after the dead event/bus cleanup. It separates live production events from underused events that still exist because they line up with current direct behavior that may be eventified next.

## Runtime Spine

The active combat chain is:

```text
seerin::Beat -> PartyTick -> ATB -> BecameActive -> skill scheduling -> FinishedTurn
```

In source terms:

1. `GrandCentral::main_loop(...)` emits `seerin::Beat` on the root `seerin::BeatBus`.
2. `PartyData::hook_to_beat(...)` receives that beat and advances `PartyLoopMachine`.
3. `PartyLoopMachine` sends `fl::fsm::NextEvent` into `PartyLoop`.
4. `PartyLoop::Ops::combat_tick(...)` emits `fl::events::PartyTick` while the party is in combat.
5. `EncounterData` listens for `PartyTick` and emits `seerin::Beat` into `AtbEngine`.
6. `AtbEngine` advances charge, emits/handles `BecameReady`, then emits `BecameActive` when a combatant can act.
7. `EncounterData` listens for `BecameActive`, chooses target/skill, and delegates to `SkillSequencer`.
8. The scheduled action emits `seerin::FinishedTurn` when the active turn is complete.

## SML Events

### `fl::fsm::NextEvent`

Defined in `src/fl/fsm/party_loop.hpp`.

Triggered by `PartyLoopMachine::beat_event()` once per forwarded party beat. Handled by `PartyLoop` to move through idle, farming, combat, town recovery, gearing, and back to idle.

### `fl::fsm::PartyWipedEvent`

Defined in `src/fl/fsm/party_loop.hpp`.

Triggered when `PartyLoopMachine` receives `fl::events::PartyWiped` from the party bus. Handled by `PartyLoop` from every current state, transitioning to `Dead`.

### `seerin::BeatTick`

Defined in `src/sr/atb_events.hpp`.

Internal ATB FSM event. `AtbEngine::on_beat(...)` sends it to each combatant that is valid and allowed to charge.

## Live Production Events

### `seerin::Beat`

Defined in `src/sr/atb_events.hpp`.

Emitted by `GrandCentral`, forwarded by `PartyData`, and emitted by `EncounterData` into ATB input when a party combat tick occurs. Observed by `PartyData` and `AtbEngine`.

### `fl::events::PartyTick`

Defined in `src/fl/events/party_bus.hpp`.

Emitted by `PartyLoop::Ops::combat_tick(...)`. Observed by `EncounterData`, which converts it to ATB `seerin::Beat`.

### `fl::events::PartyWiped`

Emitted by `TakeDamage::commit(...)` when the final living party member dies. Observed by `PartyLoopMachine`, status cleanup listeners, and pending-skill retention logic.

### `fl::events::PartyVictory`

Emitted by `PartyData::leave_combat()` when combat ends and party members are still alive. Observed by pending-skill retention logic.

### `fl::events::PartyLeftCombat`

Emitted by `PartyData::leave_combat()` before encounter teardown. Observed by status cleanup listeners.

### `fl::events::PartyGainedLevel`

Emitted by `TrackXP::add_xp(...)`. Observed by `MemberData::hook_level_progression(...)`, which grants permanent max HP.

### `fl::events::PartyHealed`

Emitted by `PartyData::revitalize_members()` after town recovery updates HP/MP. It has test listeners today, but no production listener beyond the state change that already happened before the emit.

### `fl::events::PartyRevitalizeRequested`

Emitted by `PartyLoop::Ops::fixing_tick(...)` when town recovery is complete. Observed by `PartyData`, which revitalizes members.

### `fl::events::LootDropRequested`

Emitted by `TakeDamage::commit(...)` when a party member kills an enemy. Observed by `LootDropSystem::bind_listener(...)`.

### `fl::events::PoisonApplied`

Emitted by `SkillSequencer::schedule_poison(...)`. Observed by `PoisonSystem::bind_apply_listener(...)`.

### `fl::events::FreezeApplied`

Emitted by `SkillSequencer::schedule_cold_snap(...)`. Observed by `FreezeSystem::bind_apply_listener(...)`.

### `fl::events::FreezeStarted` and `FreezeEnded`

Emitted by `FreezeSystem`. Observed by `EncounterData`, which forwards them to ATB as `seerin::Frozen` and `seerin::Thawed`.

### `fl::events::FleeAttempted` and `CombatantFled`

Emitted by `SkillSequencer::schedule_flee(...)`. Tests currently assert them; production gameplay behavior is mostly performed in the emitting callback.

### `fl::events::PlayerDied`

Emitted by `TakeDamage::commit(...)` when a previously alive party member dies. Observed by status cleanup through `StatusEffectLifetime`.

### `seerin::AddCombatant`

Emitted by `EncounterBuilder` when party members and enemies enter an encounter. Handled by `AtbEngine::on_add(...)`.

### `seerin::FinishedTurn`

Emitted by `EncounterData` when no valid target exists and by `SkillSequencer` finish callbacks. Handled by `AtbEngine::on_finished_turn(...)`.

### `seerin::Frozen` and `Thawed`

Emitted by `EncounterData` in response to `FreezeStarted`/`FreezeEnded`. Handled by `AtbEngine` and combatant ATB machines.

### `seerin::BecameReady`

Emitted by `AtbMachine` when charge fills. Handled by `AtbEngine::on_became_ready(...)`.

### `seerin::BecameActive`

Emitted by `AtbEngine::pump_ready_queue()`. Observed by `EncounterData::innervate_event_system()`.

### `fl::primitives::LogEvent`

Defined in `src/fl/primitives/logging.hpp`. Emitted by `Logger`; observed by `FancyLogSink`.

## Intentionally Underused Events

These events match real current behavior that still writes logs or mutates state directly.

### `fl::events::PartyCreated`

Defined but not emitted. Party construction currently logs directly.

### `fl::events::MemberJoined`

Defined but not emitted. Member creation currently logs directly.

### `fl::events::PartyGainedXP`

Defined but not emitted. `GrantXPToParty::commit(...)` currently logs and applies XP directly.

## Scheduler Payloads

### `seerin::TimedScheduler<AtbOutEvent>::EmitEvent`

Typed scheduled event wrapper in `src/sr/timed_scheduler.hpp`. The API exists, but current skill code mostly uses callbacks.

### `seerin::TimedScheduler<AtbOutEvent>::SmellyCallback`

The active scheduling tool for skill visuals, delayed damage, status ticks, and turn completion. Callback notes are part of the current debugging surface.

