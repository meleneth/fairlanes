# Seerin

[Seerin](https://github.com/meleneth/fairlanes/blob/89571fb5f3e6396d5e191a557602c2b25c79a31c/tests/seerin/test_atb_full_loop.cpp#L47) is the abstraction for the **ATB (Active Time Battle) system** in Fairlanes.

## What it is

Seerin is the system responsible for:

- time progression
- turn readiness
- action scheduling
- driving combat flow

It is the thing that answers:

> “who gets to act next, and when?”

Only one combatatant can be 'active' at a time, and when they signal they are finished with a ```seerin::FinishedTurn{entity_id}``` the next ready combatant (if there is one) will become active

## Where to look
- `entt::entity{}` is **not a real entity**. It’s the null sentinel (`entt::null`).

  If you see it in the test, read it as “no entity,” not “mystery entity.”
- [Full loop example](https://github.com/meleneth/fairlanes/blob/89571fb5f3e6396d5e191a557602c2b25c79a31c/tests/seerin/test_atb_full_loop.cpp#L47)

This test is the best entry point. It shows the system actually running instead of describing it abstractly.

## What to notice

- Seerin is exercised through a **full loop test**, not piecemeal unit calls
- Time and readiness are treated as **data flowing through the system**, not hidden state
- Combat progression is **driven**, not polled ad hoc
- The system integrates with event flow instead of directly mutating everything

## Why it’s called Seerin

Because naming things “ATBSystem” is how you end up with five of them and no idea which one matters.

Seerin is a single, specific abstraction with a clear responsibility.

## Why it matters here

Combat is one of the most complex flows in the codebase.

Seerin centralizes:
- timing
- ordering
- readiness

Without it, those concerns would leak across:
- entities
- systems
- event handlers

…and you’d get inconsistent behavior almost immediately.

## Fairlanes take

Combat flow should be owned by a **single coherent abstraction**, not smeared across the codebase.

## Implication for contributors

If you are adding or modifying combat timing or turn logic:

> you are probably working in Seerin, whether you realize it or not
