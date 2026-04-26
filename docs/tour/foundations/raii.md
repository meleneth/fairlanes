# RAII

**RAII** stands for **Resource Acquisition Is Initialization**.

In C++, RAII is the discipline of tying resource ownership to object lifetime. A resource is acquired during object construction and released during object destruction. This makes cleanup automatic, local, and difficult to forget.

RAII is one of the core techniques that makes C++ survivable at scale.

## What counts as a resource

A resource is anything that must be acquired and later released correctly, such as:

- heap memory
- file handles
- sockets
- locks
- transactions
- temporary registrations
- subscriptions
- scoped state changes

RAII is not just about memory. It is about **lifetime-bound correctness**.

## Why it matters

RAII turns cleanup from a matter of discipline into a matter of structure.

Without RAII, code often relies on manually paired operations:

- open / close
- lock / unlock
- subscribe / unsubscribe
- begin / end

That style is fragile. It breaks under:

- early returns
- exceptions
- branching complexity
- maintenance changes
- partial failure

RAII gives cleanup a home: the destructor.

## Example

```cpp
{
    std::lock_guard<std::mutex> lock(mutex_);
    update_shared_state();
}
```

The mutex is unlocked automatically when `lock` leaves scope.

The point is not convenience. The point is that unlocking cannot be accidentally skipped by a future change.

## RAII in Fairlanes

In Fairlanes, RAII should be preferred anywhere correctness depends on cleanup happening reliably at scope exit.

Typical uses include:

- lock management
- temporary ownership of runtime objects
- scoped registrations or subscriptions
- cleanup of transient infrastructure
- rollback or restoration of temporary state

The design goal is that resource cleanup should be **structural**, not dependent on remembering to call the matching function later.

## Design guidance

Prefer RAII when:

- a resource has a clear owner
- cleanup must happen exactly once
- cleanup must happen even on failure
- lifetime can be modeled with scope

Avoid designs that require callers to manually remember cleanup in a separate step.

If an API requires this:

```cpp
start();
...
stop();
```

ask whether that relationship should instead be represented as an owning object with a destructor.

## Do not confuse with

RAII is not:

- garbage collection
- "smart pointers everywhere"
- only about memory
- a substitute for thinking about ownership

Smart pointers are one tool that can participate in RAII, but RAII is the larger discipline.

## Related concepts

- [Lambdas](./lambdas.md)
- [Unit Testing](./unit-testing.md)
- [FSM](./fsm.md)
