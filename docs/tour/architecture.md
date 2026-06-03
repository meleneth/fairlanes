# Architecture

Fairlanes is a C++20 autobattler experiment built around ECS data, FSM-driven flow, and a small set of live event buses.

## Primary Pieces

- **ECS (EnTT)** defines what exists: entities and components.
- **FSMs (SML / Seerin / PartyLoop)** define how time and combat progress.
- **Event buses (eventpp through `seerin::VariantBus`)** define the few places where systems need decoupled signals.

## Reading Order

If you are trying to understand behavior:

1. Read the data: ECS components and owner objects.
2. Read the flow: `PartyLoop`, `PartyLoopMachine`, and Seerin ATB.
3. Read the signals: `PartyBus`, `CombatantBus`, and ATB input/output events.

## Current Combat Shape

The runtime spine is:

```text
seerin::Beat -> PartyTick -> ATB -> BecameActive -> skill scheduling -> FinishedTurn
```

Most skill effects still apply through scheduled callbacks and direct system calls. Events are used where multiple systems need to observe the same fact: party wipe, combat exit, status application, freeze/thaw, loot requests, and ATB readiness/activation.

## Fairlanes Take

Use events when a gameplay fact needs observers. Use direct calls when there is one clear owner. Delete stubs that are not part of the current runtime.
