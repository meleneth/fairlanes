# Lambdas

A **lambda** is an anonymous function object.

In modern C++, lambdas are a concise way to define callable behavior inline, often close to the point where that behavior is used.

They are especially useful for:

- callbacks
- small policy injections
- predicates
- event handlers
- local transformation logic
- scoped customization

## Capture matters

The most important part of a lambda is often not the body. It is the **capture list**.

The capture list defines what outside state the lambda depends on.

Examples:

```cpp
[value] { return value + 1; }
[&state] { state.advance(); }
[this] { handle_event(); }
```

A lambda with a careless capture list can smuggle coupling and lifetime hazards into otherwise readable code.


## Lifetime hazards

Lambdas become dangerous when they outlive what they capture.

Common problems include:

- capturing `this` when the callable may survive the object
- capturing references into asynchronous or deferred work
- hiding ownership assumptions inside event registration

When lambdas are stored, deferred, or subscribed, lifetime must be explicit.

This is where [RAII](./raii.md), ownership, and event architecture all collide.

## Lambdas in state and event code

In event-driven and FSM-heavy code, lambdas are often used to express:

- transition actions
- transition guards
- event handlers
- local routing logic

This can be elegant when kept tight.

It becomes muddy when the lambda body turns into an unnamed subsystem.

If the lambda starts carrying major policy, give it a proper home.

## Testing implications

Small local lambdas usually do not need direct tests. Their behavior is covered through the unit that owns them.

If the lambda contains meaningful business or coordination logic, that is often a smell that it should be promoted into a named function, object, or system that can be tested more directly.

## Related concepts

- [RAII](./raii.md)
- [Event-Driven Architecture](./event-driven.md)
- [FSM](./fsm.md)
- [Unit Testing](./unit-testing.md)
