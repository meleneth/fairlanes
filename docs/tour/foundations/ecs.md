# ECS

**ECS** stands for **Entity Component System**.

It is an architectural style in which behavior is organized around systems operating over entities described by components.

At a high level:

- **entities** are identities
- **components** are data
- **systems** are logic

This separates data layout from behavior and favors composition over inheritance.

## Why people use it

ECS is often used when a system needs:

- many independently varying behaviors
- flexible composition
- efficient iteration over structured data
- reduced coupling between object identity and implementation inheritance

It is especially common in simulation, games, and runtime orchestration code.

## Core terms

## Entity

An **entity** is an identity. By itself, it usually has little or no behavior.

An entity is meaningful because of the components attached to it.

## Component

A **component** is a unit of data attached to an entity.

A component should usually be:

- small
- explicit
- focused
- free of unrelated policy

A component is not a "bag of whatever."

## System

A **system** is logic that operates on entities having particular component sets.

A good system does one kind of work over well-defined data.

## Why it matters in Fairlanes

Fairlanes uses [EnTT](https://github.com/skypjack/entt) and borrows ECS structure for organizing runtime state and behavior.

That means the important mental model is usually not:

> which subclass owns this method?

It is more often:

> which data is present, and which system acts on it?

This changes how the code should be read.

When working in ECS-style code, first identify:

1. the relevant entity or entities
2. the components involved
3. the system responsible for acting on them
4. the events or state changes that trigger that work

## Design guidance

Prefer ECS-style decomposition when:

- behavior varies by attached data
- composition is more stable than inheritance
- iteration over sets of similarly-shaped state is important
- the runtime model is easier to understand as data plus transforms

Be cautious when:

- components become mini-objects with hidden policy
- systems grow broad and vague
- too much implicit coupling accumulates between distant systems
- the code stops being easier to reason about than a direct model would be

ECS is a tool, not a religion.

## What not to do

Do not treat "ECS" as a magic label that excuses bad boundaries.

Bad ECS usually looks like:

- giant components
- systems that know too much
- naming that hides ownership
- implicit ordering dependencies no one documented

If a system only works because everyone remembers a fragile ordering rule, that rule should become explicit.

## Fairlanes-specific reading guidance

When reading ECS-oriented code in Fairlanes:

- look for the relevant EnTT registry usage
- identify component types before tracing behavior
- find the system that performs the actual transformation
- watch for event flow that coordinates between systems

## Related concepts

- [Event-Driven Architecture](./event-driven.md)
- [FSM](./fsm.md)
- [Lambdas](./lambdas.md)
