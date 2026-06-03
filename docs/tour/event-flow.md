# Event Flow

Back to [Tour Home](./index.md)

For the complete current event inventory, see [Event Reference](./event-reference.md).

## Living Combat Spine

The production combat loop is deliberately narrow:

```text
seerin::Beat
  -> PartyData::hook_to_beat(...)
  -> PartyLoopMachine::beat_event()
  -> PartyLoop::Ops::combat_tick(...)
  -> fl::events::PartyTick
  -> EncounterData
  -> seerin::Beat into AtbEngine
  -> seerin::BecameActive
  -> SkillSequencer scheduling
  -> seerin::FinishedTurn
```

Concrete source trail:

- `src/fl/grand_central.cpp`: emits global `seerin::Beat` during the main loop.
- `src/fl/primitives/party_data.cpp`: listens to the global beat, forwards party beat state, and advances `PartyLoopMachine`.
- `src/fl/fsm/party_loop.cpp`: emits `fl::events::PartyTick` only while a party is farming and in combat.
- `src/fl/primitives/encounter_data.cpp`: converts `PartyTick` to ATB input, listens for `seerin::BecameActive`, chooses a target/skill, and schedules the action.
- `src/fl/skills/skill_sequence.cpp`: schedules visual/action callbacks and emits `seerin::FinishedTurn` when the active combatant is done.
- `src/sr/atb_engine.cpp`: owns ATB charge, readiness, active-combatant selection, and turn completion.

## Status And Cleanup Branches

Skill scheduling can emit combatant events such as `PoisonApplied`, `FreezeApplied`, `FleeAttempted`, and `CombatantFled` through per-combatant buses. `PoisonSystem` and `FreezeSystem` listen to their apply events and own the ECS status changes.

Damage resolution is still mostly direct: skills call `TakeDamage::commit(...)`. When damage kills a party member, `TakeDamage` emits `PlayerDied`; if the whole party is down, it emits `PartyWiped`. Status lifetimes listen to `PlayerDied`, `PartyLeftCombat`, and `PartyWiped` for cleanup.

