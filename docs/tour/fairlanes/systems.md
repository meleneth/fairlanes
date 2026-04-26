# ECS Systems

ECS Systems are encapsulations of **how things happen**.

In contrast to ECS entities and components:
- components hold data
- systems define behavior over that data

A system typically operates on entities that have a specific set of components and applies game logic to them.

## Where we use it

Two good entry points:

- [TakeDamage](https://github.com/meleneth/fairlanes/blob/89571fb5f3e6396d5e191a557602c2b25c79a31c/src/fl/ecs/systems/take_damage.cpp#L15) handles applying damage to an entity
- [GrantXpToParty](https://github.com/meleneth/fairlanes/blob/89571fb5f3e6396d5e191a557602c2b25c79a31c/src/fl/ecs/systems/grant_xp_to_party.cpp#L12) handles distributing XP across a party

## What to notice

- ECS Systems operate on **sets of components**, not specific classes
- They are usually stateless or only depend on the provided context
- They express gameplay rules explicitly instead of hiding them inside objects
- They often act as the bridge between:
  - ECS data
  - event flow
  - higher-level gameplay logic

## Why it matters here

Without systems, behavior tends to leak into:
- components (which should only hold data)
- or monolithic objects (which become hard to reason about)

Systems keep:
- data simple
- behavior centralized
- logic testable

## Fairlanes take

Components describe **what something is**.  
Systems describe **what happens to it**.

## Implication for contributors

If you are adding logic that:
- reads components
- modifies components
- or reacts to gameplay events

> it probably belongs in a ECS System, not in the component itself
