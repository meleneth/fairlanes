# Architecture

ECS-based FSMs and event-driven wiring, forming a highly maintainable skeleton that is still in need of some organs.

## What that means

Fairlanes is built from three primary pieces:

- **ECS (EnTT)** — defines what exists (entities and their data)
- **FSMs (SML / Seerin / PartyLoop)** — define how things progress over time
- **Event buses (eventpp)** — define how systems communicate

Together, they separate:
- structure (what things are)
- flow (what happens next)
- communication (who reacts)

## How to read the codebase

If you are trying to understand a behavior:

1. Look at the **data** (ECS components)
2. Look at the **flow** (FSM / Seerin / PartyLoop)
3. Look at the **signals** (event dispatch + listeners)

## Current state

The skeleton is strong:
- clear boundaries
- explicit flow
- modular data

What’s still evolving:
- richer gameplay systems
- more complete interactions between subsystems
- deeper content layered on top of the structure

## Fairlanes take

We are optimizing for **clarity of structure first**, then layering gameplay complexity on top.

The bones are in place. The muscles come next.
