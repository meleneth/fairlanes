# Seerin Combat Engine

Back to [Tour Home](../index.md)

Seerin is Fairlanes' ATB engine: it owns combat timing, readiness,
active-turn selection, freeze/thaw handling, and scheduled combat work.

The short version:

```text
seerin::Beat -> BeatTick -> BecameReady -> BecameActive -> FinishedTurn
```

Only one combatant may be active at a time.

## Where To Look

- `src/sr/atb_events.hpp`: public ATB input/output events and internal
  `BeatTick`.
- `src/sr/atb_engine.cpp`: event wiring, scheduler advancement, ready queue,
  active combatant handling.
- `src/sr/atb_fsm.hpp`: per-combatant charge/ready/frozen state machine.
- `tests/seerin/test_atb_full_loop.cpp`: current behavioral invariants.

`entt::entity{}` is the null sentinel (`entt::null`), not a real combatant.

## Event Vocabulary

ATB input events:

| Event | Meaning |
|-------|---------|
| `Beat` | Advance scheduler time and, if no combatant is active, advance combatant charge. |
| `AddCombatant` | Register an entity for ATB and add/replace its `AtbCharge` component. |
| `FinishedTurn` | The active combatant spent its turn; clear active state and reset its charge. |
| `Frozen` | Remove the entity from ready/active state and move its ATB machine to frozen. |
| `Thawed` | Move the entity out of frozen; if it is already full, emit ready again. |

ATB output events:

| Event | Meaning |
|-------|---------|
| `BecameReady` | A combatant's charge reached full. |
| `BecameActive` | The engine selected a ready combatant to act now. |

Internal FSM event:

| Event | Meaning |
|-------|---------|
| `BeatTick` | Sent by `AtbEngine` into each combatant's `AtbMachine` when charge is allowed to advance. |

## Beat Order

When `AtbEngine` receives `seerin::Beat`, the order is:

1. Run `scheduler_.on_beat()`.
2. If there is an active combatant, stop. Scheduled work still advanced, but no
   combatant accrues charge this beat.
3. For each registered combatant:
   - remove it if the entity/`AtbCharge` is gone
   - reset and force it out of turn if `can_charge(entity)` is false
   - otherwise send `BeatTick` to that combatant's `AtbMachine`
4. `BeatTick` increments charge by `80` uWu.
5. If the charge reaches `max_charge`, the combatant emits `BecameReady` and
   enters `Ready`.
6. `AtbEngine` hears `BecameReady`, removes any duplicate ready entry, and
   pushes the entity onto `ready_queue_`.
7. After all combatants tick, `pump_ready_queue()` selects the first
   charge-allowed ready entity, sets it active, and emits `BecameActive`.

The observable order for a newly full combatant is therefore:

```text
Beat -> BeatTick -> BecameReady -> BecameActive
```

## Turn Completion Order

Skill code or `EncounterData` emits `FinishedTurn{id}` when the active entity is
done.

`AtbEngine::on_finished_turn(...)` only applies the turn reset if `id` is
currently active:

1. clear `active_combatant_`
2. send `FinishedTurn` into that entity's `AtbMachine`
3. reset its `AtbCharge::charge` to `0`
4. return it to `Charging`

The next ready combatant is not activated by `FinishedTurn` itself. Activation
happens on the next `Beat`, after scheduled work and charge processing.

## Freeze And Thaw Order

`Frozen{id}`:

1. removes `id` from the ready queue
2. clears active state if `id` was active
3. sends `Frozen` to that combatant's `AtbMachine`
4. frozen combatants ignore `BeatTick`

`Thawed{id}`:

1. sends `Thawed` to that combatant's `AtbMachine`
2. if the combatant's charge is already full, the machine emits `BecameReady`
3. `AtbEngine` enqueues the entity again

This matters for Cold Snap/Freeze: a ready frozen combatant leaves the queue,
but if it thaws while still full it re-enters the queue.

## Current Invariants

These are protected by tests and should be treated as design constraints:

- Only one combatant can be active at a time.
- Scheduler work advances on every beat, even while a combatant is active.
- No combatant accrues ATB charge while any combatant is active.
- `BecameReady` is observed before `BecameActive` for the same readiness cycle.
- `FinishedTurn` resets the active combatant's charge to `0` and clears active
  state.
- Dead or otherwise non-chargeable combatants reset charge and do not accumulate
  until `can_charge(...)` permits it again.
- Frozen combatants do not accrue charge.
- Freezing a ready combatant removes it from the ready queue.
- Thawing a full combatant emits `BecameReady` again.
- `clear_active_turn_for(owner)` removes scheduled callbacks owned by that
  active turn without clearing unrelated effect-owned callbacks.

## How EncounterData Uses It

`EncounterData` listens for `BecameActive`. When ATB selects an active
combatant, `EncounterData` chooses a target and skill, then uses
`SkillSequencer` to schedule the action. The skill sequence eventually emits
`FinishedTurn` for that combatant.

Freeze status events are bridged in `EncounterData` too: `FreezeStarted`
becomes ATB `Frozen`, and `FreezeEnded` becomes ATB `Thawed`.

## Contributor Guidance

If a change touches turn readiness, active-combatant ownership, freeze/thaw
behavior, or time progression, it belongs in or near Seerin.

When adding tests, prefer event-order claims over implementation trivia:

```text
given this charge/state
when this ATB event arrives
then these output events and charge/active states result
```
