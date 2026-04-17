# FSM

**FSM** stands for **Finite State Machine**.

A finite state machine models behavior as a set of states and explicit transitions between them.

At any given time, the machine is in one state. Events, conditions, or commands may cause it to transition to another.

## Why it matters

FSMs are useful when behavior depends strongly on current state and when valid transitions should be explicit.

They help answer questions like:

- what state is this thing in right now?
- what transitions are allowed?
- what event caused the change?
- what actions happen during the transition?

Without a clear state model, runtime logic often degenerates into scattered booleans and partial invariants.

## Why it matters in Fairlanes

Fairlanes uses [boost-ext/sml](https://github.com/boost-ext/sml), so state-machine structure is part of the implementation model, not just a whiteboard story.

That means the right question is often not:

> why did this branch run?

It is:

> what state was active, and what transition was legal from there?

When behavior is modeled as a state machine, correctness lives in the transition model.

## Core ideas

## State

A **state** represents a mode of behavior.

Examples:

- idle
- pending
- active
- blocked
- completed
- failed

A state should reflect a meaningful behavioral distinction.

## Transition

A **transition** is a legal movement from one state to another.

A good transition is:

- explicit
- intentional
- triggered by a named event or condition

## Guard

A **guard** is a condition that must be satisfied for a transition to occur.

Guards should be narrow and readable. If a guard becomes a logic swamp, the model is trying to hide complexity instead of clarifying it.

## Action

An **action** is work performed as part of a transition.

Actions should support the state model, not undermine it.

## Design guidance

Use an FSM when:

- behavior changes materially by state
- valid transitions matter
- illegal transitions should be impossible or visible
- the system benefits from an explicit lifecycle model

Avoid hiding state in:

- loosely related flags
- scattered if statements
- undocumented ordering assumptions

A state machine is usually better than a pile of boolean folklore.

## Common failure modes

FSMs become unpleasant when:

- states are too vague
- transitions are not named clearly
- side effects dominate the model
- the real state is still scattered elsewhere
- every edge case creates another state without improving clarity

The state machine should simplify reasoning, not merely relocate confusion.

## Reading guidance in Fairlanes

When reading FSM-based logic:

1. identify the current state
2. identify the triggering event
3. identify guards
4. identify the transition target
5. identify any transition actions
6. check for emitted events that may trigger downstream reactions

## Testing guidance

FSM-heavy code should be tested around:

- valid transitions
- invalid transitions
- guarded transitions
- action side effects
- event emission caused by state changes

A good test suite should make illegal or surprising transitions visible immediately.

## Related concepts

- [Event-Driven Architecture](./event-driven.md)
- [Unit Testing](./unit-testing.md)
- [RAII](./raii.md)
