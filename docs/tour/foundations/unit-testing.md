# Unit Testing

**Unit testing** is the practice of verifying the behavior of a small, focused unit of code in isolation.

The purpose is not to prove the entire system works. The purpose is to make local behavior explicit, verifiable, and stable under change.

In Fairlanes, unit tests are part of design discipline, not a ceremonial checkbox.

## Running the Tests
```sh
cmake --build build
ctest --test-dir build --output-on-failure
```

## Why it matters in Fairlanes

Fairlanes uses [Catch2](https://github.com/catchorg/Catch2), which supports expressive test structure without requiring the test framework to become the main character.

The important point is not the framework. The important point is that tests should clarify behavior and protect design boundaries.

## What a unit test should do

A good unit test should:

- focus on one behavior
- name the situation clearly
- assert something meaningful
- avoid unnecessary setup
- fail for a clear reason

The test should make the unit's contract easier to understand.

## What a unit test should not do

A unit test should not:

- test half the system accidentally
- require elaborate global setup without need
- depend on unstable timing
- encode internal implementation trivia as the contract
- become impossible to read without tracing five fixtures

If the test is harder to understand than the code, it is not helping enough.

## Isolation

Isolation does not always mean "no collaborators." It means the test should control the factors relevant to the behavior being verified.

That may involve:

- test doubles
- local registries
- explicit test state
- narrow fixtures
- deterministic event setup

The goal is clarity and repeatability.

## Design feedback

Tests are not just verification. They are design feedback.

If a unit is hard to test, the problem may be:

- too many responsibilities
- hidden dependencies
- unclear ownership
- poor state boundaries
- excessive coupling

Sometimes the test pain is the architecture speaking.

## Catch2 usage guidance

Catch2 gives the project a readable way to express focused tests. Use that readability.

Prefer tests that read like a behavioral claim:

- given this state
- when this action occurs
- then this outcome follows

Do not hide the important setup behind a mountain of magic unless that magic actually improves clarity.

## Related concepts

- [FSM](./fsm.md)
- [Event-Driven Architecture](./event-driven.md)
- [RAII](./raii.md)
- [Lambdas](./lambdas.md)
