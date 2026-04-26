# Event-Driven Architecture

An **event-driven** design reacts to things that happen rather than forcing all coordination through direct, synchronous control flow.

An **event** is a statement that something occurred.

Examples:

- an input was received
- a state transition completed
- an entity was created
- a timer elapsed
- a command finished
- a failure was observed

In an event-driven system, producers emit events and consumers react to them.

## Why people use it

Event-driven structure is useful when a system needs:

- loose coupling between producers and consumers
- extensibility
- clear reaction points
- decoupled coordination between subsystems
- a runtime model based on change and reaction

This style is common in UI systems, simulation, orchestration, and messaging-heavy applications.

## Why it matters in Fairlanes

Fairlanes uses [eventpp](https://github.com/wqking/eventpp), which means event flow is not incidental plumbing. It is part of the architecture.

The mental model is often:

1. some change or action occurs
2. an event is published
3. one or more consumers respond
4. downstream state changes or transitions occur

That means understanding behavior often requires following events, not just following direct function calls.

## Good event design

A good event is:

- clearly named
- specific
- meaningful in domain or runtime terms
- narrow enough to avoid ambiguity
- stable enough that consumers can rely on its meaning

Examples of better event names:

- `entity_spawned`
- `route_completed`
- `lane_blocked`

Examples of worse event names:

- `updated`
- `changed`
- `thing_happened`

If the event name hides the actual meaning, readers are forced to reverse-engineer intent from subscribers.

## Event-driven does not mean unstructured

A common mistake is to treat event-driven architecture as an excuse for invisible control flow.

A healthy event-driven system still needs:

- clear ownership
- named event contracts
- explicit sequencing where order matters
- documentation of important producers and consumers
- restraint

Not every interaction should become an event.

## When to use events

Use events when:

- one action should be observable by multiple consumers
- the producer should not know every downstream reaction
- the system benefits from decoupled reactions
- the event has durable meaning in the runtime model

Do not use events just to avoid making a direct dependency decision.

## Failure modes

Event-driven systems often become hard to reason about when:

- too many vague events exist
- ordering matters but is undocumented
- event chains become long and implicit
- debugging requires guesswork
- the same concept appears under several similar names

If behavior depends on event ordering, make that fact explicit in code and docs.

## Reading guidance in Fairlanes

When tracing behavior:

1. identify the event source
2. find the event type
3. identify subscribers
4. determine which state or components change in response
5. check whether an [FSM](./fsm.md) transition is involved

## Related concepts

- [ECS](./ecs.md)
- [FSM](./fsm.md)
- [Unit Testing](./unit-testing.md)
