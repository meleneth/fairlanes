# eventpp

We use [eventpp](https://github.com/wqking/eventpp) as the foundation for our event bus system.

In Fairlanes, eventpp is mostly used indirectly through our own bus types rather than referenced everywhere by name.

## What it is

eventpp provides the machinery for publishing events and attaching listeners.

It is the low-level library behind patterns like:

- define an event vocabulary
- dispatch an event with a payload
- register listeners that react to that event

In Fairlanes, that gives us a way to move gameplay-relevant signals through the system without hard-wiring every subsystem directly to every other subsystem.

## Where we use it

The clearest entry points are our bus typedefs:

- [`PartyBus`](https://github.com/meleneth/fairlanes/blob/89571fb5f3e6396d5e191a557602c2b25c79a31c/src/fl/events/party_bus.hpp)
- [`AccountBus`](https://github.com/meleneth/fairlanes/blob/89571fb5f3e6396d5e191a557602c2b25c79a31c/src/fl/events/account_bus.hpp)

These are built on `eventpp::EventDispatcher`.

## What to notice

Fairlanes does not generally spread raw `eventpp::...` usage everywhere.

Instead, we wrap it in project-level types like `PartyBus` and `AccountBus`, which means the interesting layer is usually:

- what events exist
- where they are dispatched
- where listeners are attached

That makes the code easier to read in domain terms instead of library terms.

## How to explore usage

Searching for `eventpp` directly may not show much.

Better project-wide searches are:

- `PartyBus`
- `AccountBus`
- `PartyEvent`
- `AccountEvent`
- `appendListener`
- `dispatch(`

Those searches will show:

- where event buses are defined
- where events are emitted
- where systems react to them

A good mental model is:

```text
event enum -> dispatch() -> appendListener()
```

- the enum defines the event vocabulary
- `dispatch()` shows who speaks
- `appendListener()` shows who reacts

## Why it matters here

Without an event system, gameplay code tends to collapse into direct calls and hidden coupling.

With event buses, Fairlanes can express gameplay flow in terms of meaningful signals like:

- party events
- account events
- combat-relevant changes

That helps keep systems decoupled while still letting them coordinate.

## Fairlanes take

We use eventpp as infrastructure, not as the star of the show.

The important thing is not “this is eventpp.”
The important thing is that Fairlanes has explicit event vocabularies and scoped buses that let gameplay systems communicate without turning into a pile of direct dependencies.

## Implication for contributors

If you want to understand event flow, do not start by searching for `eventpp`.

Start with:
- the bus typedefs
- the event enums
- `dispatch()`
- `appendListener()`

That is where the real shape of the system shows up.
